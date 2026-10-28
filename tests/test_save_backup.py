import datetime
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import paths  # noqa: F401  (puts scripts/ on sys.path)
import save_backup


def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding='utf-8')


class SaveBackupTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.user = Path(self.temp.name)
        self.save = self.user / 'savedata' / '1' / 'CUSA01623' / 'AUTOSAVE' / 'data.bin'
        self.clock = datetime.datetime(2026, 10, 9, 18, 0, 0)

    def tearDown(self):
        self.temp.cleanup()

    def tick(self):
        self.clock += datetime.timedelta(minutes=1)
        return self.clock

    def test_copies_savedata(self):
        write(self.save, 'v1')
        made = save_backup.backup(self.user, now=self.tick())
        self.assertEqual(made.name, '20261009-180100')
        self.assertEqual((made / self.save.relative_to(self.user / 'savedata')).read_text(), 'v1')

    def test_no_backup_without_saves(self):
        self.assertIsNone(save_backup.backup(self.user, now=self.tick()))
        (self.user / 'savedata').mkdir()
        self.assertIsNone(save_backup.backup(self.user, now=self.tick()))
        self.assertEqual(save_backup.list_backups(self.user), [])

    def test_partial_backup_copy_not_published_or_restorable(self):
        write(self.save, 'current progress')
        from shutil import copytree as actual_copytree

        def interrupt_copy(src, dest, *args, **kwargs):
            if Path(dest).name == 'payload':
                partial = Path(dest) / self.save.relative_to(self.user / 'savedata')
                write(partial, 'incomplete backup')
                raise OSError('simulated interrupted backup write')
            return actual_copytree(src, dest, *args, **kwargs)

        with mock.patch.object(save_backup.shutil, 'copytree', side_effect=interrupt_copy):
            with self.assertRaisesRegex(OSError, 'interrupted backup write'):
                save_backup.backup(self.user, now=self.tick())
        self.assertEqual(self.save.read_text(), 'current progress')
        self.assertEqual(save_backup.list_backups(self.user), [])
        self.assertFalse(list((self.user / 'savedata_backups').glob(
            '.savedata-backup-*')))

    def test_stale_interrupted_backup_is_hidden_not_deleted(self):
        write(self.save, 'current progress')
        stale = (self.user / 'savedata_backups' / '.savedata-backup-old'
                 / 'payload' / self.save.relative_to(self.user / 'savedata'))
        write(stale, 'partial old data')
        made = save_backup.backup(self.user, now=self.tick())
        self.assertEqual(save_backup.list_backups(self.user), [made.name])
        self.assertEqual(stale.read_text(encoding='utf-8'), 'partial old data')

    def test_same_second_gets_unique_name(self):
        write(self.save, 'v1')
        first = save_backup.backup(self.user, now=self.clock)
        second = save_backup.backup(self.user, now=self.clock)
        self.assertNotEqual(first, second)
        self.assertEqual(len(save_backup.list_backups(self.user)), 2)

    def test_same_second_backups_sort_by_creation_sequence(self):
        write(self.save, 'v1')
        made = [save_backup.backup(self.user, now=self.clock, keep=20) for _ in range(12)]
        self.assertEqual(save_backup.list_backups(self.user), [p.name for p in reversed(made)])

    def test_same_second_retention_keeps_latest_contents_and_unique_names(self):
        made = []
        for index in range(14):
            write(self.save, f'version {index}')
            made.append(save_backup.backup(self.user, now=self.clock))
            self.assertTrue(made[-1].is_dir(), 'the backup just created must survive pruning')
        self.assertEqual(len(set(made)), len(made))
        expected = list(reversed(made[-save_backup.KEEP:]))
        self.assertEqual(save_backup.list_backups(self.user), [p.name for p in expected])
        relative = self.save.relative_to(self.user / 'savedata')
        self.assertEqual([(p / relative).read_text() for p in expected],
                         [f'version {i}' for i in range(13, 3, -1)])

    def test_keeps_newest_ten(self):
        write(self.save, 'v')
        made = [save_backup.backup(self.user, now=self.tick()) for _ in range(12)]
        kept = save_backup.list_backups(self.user)
        self.assertEqual(len(kept), save_backup.KEEP)
        self.assertEqual(kept[0], made[-1].name)  # newest first
        self.assertFalse(made[0].exists())
        self.assertFalse(made[1].exists())

    def test_same_second_restore_keeps_the_safety_backup(self):
        for index in range(12):
            write(self.save, f'version {index}')
            save_backup.backup(self.user, now=self.clock)
        oldest = save_backup.list_backups(self.user)[-1]
        write(self.save, 'current progress')
        safety = save_backup.restore(self.user, oldest, now=self.clock)
        relative = self.save.relative_to(self.user / 'savedata')
        self.assertEqual(self.save.read_text(), 'version 2')
        self.assertEqual((safety / relative).read_text(), 'current progress')
        self.assertEqual(save_backup.list_backups(self.user)[0], safety.name)
        self.assertEqual(len(save_backup.list_backups(self.user)), save_backup.KEEP)

    def test_restore_backs_up_current_first(self):
        write(self.save, 'old')
        old = save_backup.backup(self.user, now=self.tick())
        write(self.save, 'new')
        write(self.save.parent / 'extra.bin', 'x')
        safety = save_backup.restore(self.user, old.name, now=self.tick())
        self.assertEqual(self.save.read_text(), 'old')
        self.assertFalse((self.save.parent / 'extra.bin').exists())
        self.assertEqual((safety / self.save.relative_to(self.user / 'savedata')).read_text(), 'new')

    def test_restore_oldest_when_full(self):
        write(self.save, 'oldest')
        oldest = save_backup.backup(self.user, now=self.tick())
        write(self.save, 'later')
        for _ in range(save_backup.KEEP - 1):
            save_backup.backup(self.user, now=self.tick())
        save_backup.restore(self.user, oldest.name, now=self.tick())
        self.assertEqual(self.save.read_text(), 'oldest')
        self.assertEqual(len(save_backup.list_backups(self.user)), save_backup.KEEP)

    def test_failed_stage_copy_never_moves_or_deletes_active_saves(self):
        write(self.save, 'original backup')
        old = save_backup.backup(self.user, now=self.tick())
        write(self.save, 'current progress')
        from shutil import copytree as real_copytree

        def fail_stage(src, dest, *args, **kwargs):
            if Path(dest).name == 'incoming':
                raise OSError('simulated incomplete stage copy')
            return real_copytree(src, dest, *args, **kwargs)

        with mock.patch.object(save_backup.shutil, 'copytree', side_effect=fail_stage):
            with self.assertRaisesRegex(OSError, 'incomplete stage copy'):
                save_backup.restore(self.user, old.name, now=self.tick())
        self.assertEqual(self.save.read_text(), 'current progress')
        self.assertTrue((old / self.save.relative_to(self.user / 'savedata')).is_file())
        self.assertFalse(list(self.user.glob('.savedata-restore-*')))

    def test_failed_activation_rolls_back_without_losing_current_saves(self):
        write(self.save, 'original backup')
        old = save_backup.backup(self.user, now=self.tick())
        write(self.save, 'current progress')
        real_rename = Path.rename

        def fail_incoming(path, target):
            if path.name == 'incoming':
                raise OSError('simulated failed activation')
            return real_rename(path, target)

        with mock.patch.object(Path, 'rename', fail_incoming):
            with self.assertRaisesRegex(OSError, 'failed activation'):
                save_backup.restore(self.user, old.name, now=self.tick())
        self.assertEqual(self.save.read_text(), 'current progress')
        self.assertFalse(list(self.user.glob('.savedata-restore-*')))
        # A backup of the interrupted current progress still exists.
        contents = [(self.user / 'savedata_backups' / name
                     / self.save.relative_to(self.user / 'savedata')).read_text()
                    for name in save_backup.list_backups(self.user)]
        self.assertIn('current progress', contents)

    def test_failed_rollback_preserves_recoverable_original_directory(self):
        write(self.save, 'original backup')
        old = save_backup.backup(self.user, now=self.tick())
        write(self.save, 'current progress')
        real_rename = Path.rename

        def fail_activation_and_rollback(path, target):
            if path.name in ('incoming', 'previous'):
                raise OSError('simulated locked directory')
            return real_rename(path, target)

        with mock.patch.object(Path, 'rename', fail_activation_and_rollback):
            with self.assertRaisesRegex(RuntimeError, 'original saves preserved at'):
                save_backup.restore(self.user, old.name, now=self.tick())
        recovery = list(self.user.glob('.savedata-restore-*/previous'))
        self.assertEqual(len(recovery), 1)
        relative = self.save.relative_to(self.user / 'savedata')
        self.assertEqual((recovery[0] / relative).read_text(), 'current progress')
        # Recovery data is deliberately retained for an explicit manual fix.
        self.assertTrue((self.user / 'savedata_backups').is_dir())

    def test_success_does_not_delete_stale_legacy_staging(self):
        write(self.save, 'old')
        old = save_backup.backup(self.user, now=self.tick())
        write(self.save, 'current')
        stale = self.user / 'savedata.restoring' / 'manual-restore.bin'
        write(stale, 'important previous attempt')
        save_backup.restore(self.user, old.name, now=self.tick())
        self.assertEqual(self.save.read_text(), 'old')
        self.assertEqual(stale.read_text(), 'important previous attempt')
        self.assertFalse(list(self.user.glob('.savedata-restore-*')))

    def test_restore_rejects_unknown_or_unsafe_names(self):
        write(self.save, 'v')
        for name in ('missing', '..', '../savedata', ''):
            with self.assertRaises(ValueError):
                save_backup.restore(self.user, name, now=self.tick())
        self.assertEqual(self.save.read_text(), 'v')


if __name__ == '__main__':
    unittest.main()
