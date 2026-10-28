"""Portable launcher migration must never delete a user's legacy settings.

No actual Tk window, Windows profile, game dump or user saves are needed.
"""
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from paths import ROOT

sys.path.insert(0, str(ROOT / 'launcher'))
import port_launcher_win as launcher  # noqa: E402


class LegacyMigrationSafetyTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.data = self.root / 'portable'
        self.data.mkdir()
        self.appdata = self.root / 'AppData'
        self.legacy = []
        for name in ('theorder1886-launcher', 'order1886-launcher'):
            folder = self.appdata / name
            folder.mkdir(parents=True)
            sentinel = folder / 'hand-edited-user-settings.json'
            sentinel.write_text('keep-my-original-settings', encoding='utf-8')
            self.legacy.append(sentinel)
        for name, value in (
            ('DATA_DIR', self.data),
            ('CONFIG_DIR', self.data),
            ('CONFIG_FILE', self.data / 'settings.json'),
        ):
            patch = mock.patch.object(launcher, name, value)
            patch.start()
            self.addCleanup(patch.stop)
        env = mock.patch.dict(os.environ, {'APPDATA': str(self.appdata)})
        env.start()
        self.addCleanup(env.stop)
        platform_flag = mock.patch.object(launcher, 'WINDOWS', True)
        platform_flag.start()
        self.addCleanup(platform_flag.stop)

    def migrate(self, version):
        app = launcher.Launcher.__new__(launcher.Launcher)
        app.app = {'last_version': version, 'ui_language': 'en'}
        app.check_version_migration()
        return app

    def assert_legacy_intact(self):
        for sentinel in self.legacy:
            self.assertTrue(sentinel.is_file(), str(sentinel))
            self.assertEqual(sentinel.read_text(encoding='utf-8'),
                             'keep-my-original-settings')

    def test_initial_migration_copies_ini_but_preserves_originals(self):
        source = self.data / 'order1886.ini'
        source.write_text('upscaler=fsr3\npreset=2\n', encoding='utf-8')
        instance = self.migrate('0.7.8')
        self.assertEqual((self.data / 'theorder.ini').read_bytes(), source.read_bytes())
        self.assertTrue(source.is_file())
        self.assert_legacy_intact()
        self.assertEqual(instance.app['last_version'], launcher.VERSION)
        saved = json.loads((self.data / 'settings.json').read_text(encoding='utf-8'))
        self.assertEqual(saved['last_version'], launcher.VERSION)
        self.migrate(launcher.VERSION)
        self.assertTrue(source.is_file())
        self.assert_legacy_intact()

    def test_existing_current_ini_is_never_overwritten_or_legacy_deleted(self):
        original = self.data / 'order1886.ini'
        current = self.data / 'theorder.ini'
        original.write_text('upscaler=fsr4\n', encoding='utf-8')
        current.write_text('upscaler=off\n', encoding='utf-8')
        self.migrate('0.7.8')
        self.assertEqual(current.read_text(encoding='utf-8'), 'upscaler=off\n')
        self.assertEqual(original.read_text(encoding='utf-8'), 'upscaler=fsr4\n')
        self.assert_legacy_intact()

    def test_failed_ini_copy_keeps_original_files(self):
        original = self.data / 'order1886.ini'
        original.write_text('custom=1\n', encoding='utf-8')
        with mock.patch.object(launcher.shutil, 'copy2', side_effect=OSError('disk busy')):
            self.migrate('0.7.8')
        self.assertFalse((self.data / 'theorder.ini').exists())
        self.assertEqual(original.read_text(encoding='utf-8'), 'custom=1\n')
        self.assert_legacy_intact()


if __name__ == '__main__':
    unittest.main()
