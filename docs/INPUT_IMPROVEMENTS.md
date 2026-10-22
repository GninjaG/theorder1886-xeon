# Input status and validation gaps

2026-10-22 source checkpoint; native camera input is NOT implemented by this work.

## Reproduced menu input-lifetime defect

The actual `PortOverlay::HandleEvent` path left ImGui keyboard/gamepad presses
and mouse clicks held when the menu closed. Releases during gameplay are not
forwarded to ImGui, so reopening could resume navigation repeat or a drag.
Queued presses not yet processed by an ImGui frame had the same lifetime bug.

`SetOpen` now clears the incoming event queue, processed keyboard/gamepad and
mouse state, and active widget when the open state changes. This applies to
Insert and both R3+L2 chord orders, without changing gameplay controller maps
or mouse sensitivity. Fresh mixed-device presses/releases while open still work.

`overlay-input-test` includes the actual overlay implementation, uses real SDL
events and the real ImGui input processor without a Vulkan backend or guest.
The close/release/reopen test was red: `keyboard=1 mouse=1 gamepad=1`.
After the fix: `keyboard=0 mouse=0 gamepad=0`. Queued-input and both gamepad
chord orders also pass. This regression is now in `tools\run_tests.sh`.
This fixes a demonstrated stuck-input cause, not every possible simultaneous
physical-device interaction or native camera responsiveness.

Rebuilt-runtime smoke: `out\checkpoint-20261022\overlay-input.json`, 70-second
isolated warm-profile run, exit 0, 3,300 flips, no reported guest fault.
`overlay-window-replay.json` records Windows messages posted only to that
runtime's HWND for the close/release/reopen sequence. This is not proof of
physical-device delivery or a visual menu check: the renderer's raw output
capture occurs before the overlay is drawn. The deterministic regression above,
not the title-screen capture, establishes the corrected input state.

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

The broader Insert / R3+L2 keyboard/mouse/gamepad interaction remains subject to the
manual checklist in `MANUAL_GAMEPLAY_TESTS.md`; simultaneous physical input was
NOT TESTED in these automated frontend/prologue runs.
