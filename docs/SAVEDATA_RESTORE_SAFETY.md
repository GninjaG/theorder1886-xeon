# Savedata backup and restore safety

`scripts/save_backup.py` backs up the user's `savedata` directory before
game startup and before any requested restore. A restore selects a named
backup already present in `savedata_backups` and copies it to a unique
`.savedata-restore-*` staging directory next to the active saves.

Backup creation is also staged: new backups first copy into a uniquely
named hidden `.savedata-backup-*` directory and become a restorable timestamp
directory only after the copy is complete. An exception removes this operation's
temporary folder; an abrupt process termination can leave a hidden partial
folder behind for manual review, but it will **not** appear in the usable
backup list or be automatically removed on the next run. Previous manually
named backups remain visible.

Restore flow:

1. Create a safety backup of the current `savedata` first. If the copy fails,
   do not modify the active saves.
2. Fully copy the requested backup into a new staging directory; no current
   file is deleted while this copy is incomplete.
3. Move the existing `savedata` into the staging directory as `previous`.
   Move the complete staged `incoming` to `savedata`.
4. If activation fails, attempt to move `previous` back to `savedata`.
   If rollback itself fails, **retain the entire staging directory** and
   report where the original saves were left for manual recovery.
5. Only after activation succeeds, remove the superseded staging directory,
   then apply the existing backup-retention policy. The safety backup is
   retained according to the same newest-ten policy.

The historical fixed `savedata.restoring` directory is no longer
automatically deleted. It could contain recoverable data from an interrupted
earlier restore and is left for an explicit user decision.

The focused regression tests simulate stage-copy failures, activation failure,
rollback failure, and a stale staging directory using disposable temporary
directories only. No real save, game or GPU is required.

A Windows filesystem failure or forced process termination can still leave
recovery work: do not treat synthetic passing tests as proof that every
filesystem, antivirus or power-loss scenario is harmless. Do **not** manually
delete a leftover `.savedata-restore-*` directory until its `previous`
contents have been compared with the active saves and backups.
