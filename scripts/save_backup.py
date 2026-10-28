#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Backups of the game's saves: <user>/savedata copied to <user>/savedata_backups/<time>.

run.py makes one before every start; the launcher lists them and restores one. A restore
first backs up the current saves, so it can always be undone.
"""
import datetime
from pathlib import Path
import re
import shutil
import tempfile

KEEP = 10
STAMP = '%Y%m%d-%H%M%S'
BACKUP_NAME = re.compile(r'([0-9]{8}-[0-9]{6})(?:-([0-9]+))?')


def backups_dir(user_dir):
    return Path(user_dir) / 'savedata_backups'


def _backup_order(name):
    match = BACKUP_NAME.fullmatch(name)
    return (match[1], int(match[2] or 1)) if match else (name, 0)


def list_backups(user_dir):
    """Backup names, newest first."""
    root = backups_dir(user_dir)
    # Ignore incomplete, hidden staging directories from interrupted copies.
    # Keep older manually named backups visible for backwards compatibility.
    return sorted((p.name for p in root.iterdir()
                   if p.is_dir() and not p.name.startswith('.savedata-backup-')),
                  key=_backup_order, reverse=True) if root.is_dir() else []


def _has_saves(savedata):
    return savedata.is_dir() and any(p.is_file() for p in savedata.rglob('*'))


def _prune(user_dir, keep=KEEP):
    for name in list_backups(user_dir)[keep:]:
        shutil.rmtree(backups_dir(user_dir) / name, ignore_errors=True)


def _copy(user_dir, now):
    savedata = Path(user_dir) / 'savedata'
    if not _has_saves(savedata):
        return None
    root = backups_dir(user_dir)
    root.mkdir(parents=True, exist_ok=True)
    base = (now or datetime.datetime.now()).strftime(STAMP)
    # Pruning may have removed the unsuffixed folder or left gaps. Keep the
    # sequence increasing so a fresh backup cannot sort behind older ones.
    n = 1
    for entry in root.iterdir():
        stamp, sequence = _backup_order(entry.name)
        if stamp == base:
            n = max(n, sequence + 1)
    target = root / (base if n == 1 else f'{base}-{n}')
    # Do not publish a partial timestamped backup if copying fails. A crash
    # can leave a hidden staging folder for manual recovery, but it will not
    # appear among the usable/restorable backups until the copy completes.
    staging = Path(tempfile.mkdtemp(prefix='.savedata-backup-', dir=root))
    try:
        incoming = staging / 'payload'
        shutil.copytree(savedata, incoming)
        incoming.rename(target)
        return target
    finally:
        # Only this operation's unique staging directory is cleaned up.
        # Never delete staging from another interrupted backup attempt.
        shutil.rmtree(staging, ignore_errors=True)


def backup(user_dir, now=None, keep=KEEP):
    """Copies savedata; returns the new folder, or None when there is nothing to save."""
    made = _copy(user_dir, now)
    if made:
        _prune(user_dir, keep)
    return made


def restore(user_dir, name, now=None):
    """Replaces savedata with backup `name`; returns the backup of the saves it replaced."""
    if not name or name not in list_backups(user_dir):
        raise ValueError(f'No save backup named {name!r}')
    source = backups_dir(user_dir) / name
    savedata = Path(user_dir) / 'savedata'
    # Preserve the original before any swap; never prune the source backup
    # until the replacement has been installed successfully.
    safety = _copy(user_dir, now)
    # A unique private staging folder avoids clearing another interrupted
    # restore's recoverable data (the old fixed savedata.restoring was deleted).
    staging_root = Path(tempfile.mkdtemp(prefix='.savedata-restore-', dir=user_dir))
    incoming = staging_root / 'incoming'
    previous = staging_root / 'previous'
    activated = False
    try:
        # A failed/incomplete copy leaves the active savedata untouched.
        shutil.copytree(source, incoming)
        if savedata.exists():
            savedata.rename(previous)
        try:
            incoming.rename(savedata)
            activated = True
        except BaseException:
            if previous.exists():
                try:
                    previous.rename(savedata)
                except OSError as error:
                    # The old saves are STILL in previous. Do not remove
                    # staging_root: its path is needed for manual recovery.
                    raise RuntimeError(
                        f'Cannot restore savedata automatically; original saves '
                        f'preserved at {previous}') from error
            raise
    finally:
        # Only clean up when the new savedata is installed or rollback
        # succeeded. On failed rollback, previous is the only active copy.
        if activated or not previous.exists():
            shutil.rmtree(staging_root, ignore_errors=True)
    _prune(user_dir)
    return safety

