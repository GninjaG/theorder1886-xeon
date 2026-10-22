# Input status and validation gaps

2026-10-22 source checkpoint; native camera input is NOT implemented by this work.

## Scripted-input reliability

The Windows `THEORDER_PAD_FILE` change detector compared whole-second `stat`
mtime plus file size. Equal-size edits within that second were dropped.
`pad-map-test` now drives the actual `read_inject` path with two controlled
subsecond file timestamps: `cross` then `right`, both five bytes, followed by
release. Before the fix it reports `0x4000 -> 0x4000 -> 0`; afterward
`0x4000 -> 0x20 -> 0`.

The runtime uses SDL's high-resolution file modification time plus size instead,
retains the 20 ms polling interval, and commits its remembered stamp only after
opening/reading the file. This fixes benchmark/control injection, not native mouse
movement or the mixed-input overlay bug. Inputs shorter than the polling interval
are still not guaranteed to be sampled.

An isolated copy of the user's save reached the prologue handcuff cinematic and,
in a longer run, the first Triangle QTE. Timed Cross presses alone do not advance
that prompt. These captures establish an input route beyond the frontend, not
a calibrated free-camera test.

## Mouse and overlay gaps

The consolidated branch preserves independent X/Y sensitivity, Y inversion,
relative SDL mouse capture, controller mappings and overlay input isolation.
`pad-map-test` covers the existing mapping/deadzone/capture-release helpers.
These are not an in-game camera calibration.

Important discrepancy: `src\runtime_pad.c::mouse_to_stick` still turns mouse
deltas into bounded virtual-stick deflections with deadzone compensation and a
nonlinear magnitude curve. Its current implementation does not consume elapsed
time. Including `mouse_stick.h` does not connect that header's velocity-window
implementation to the actual camera path. The standalone header tests do not
prove runtime FPS independence or native mouse behavior.

Do not call this 1:1/raw camera look or declare the 400% sensitivity issue fixed.
Needed evidence: repeatable gameplay camera displacement for identical raw counts
at 30/60/120 FPS, controlled polling rates, saturation tests and a safe engine-camera
integration seam. No engine hooks or guest-code patches were introduced here.

The Insert / R3+L2 keyboard/mouse/gamepad interaction remains subject to the
manual checklist in `MANUAL_GAMEPLAY_TESTS.md`; simultaneous physical input was
NOT TESTED in these automated frontend/prologue runs.
