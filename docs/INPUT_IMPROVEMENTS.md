# Input status and validation gaps

Updated 2026-10-25; native camera input is NOT implemented by this work.

## R3+L2 menu scrolling and mixed-device input

The menu now has a fixed ImGui window ID independent of its dynamic device
prompts. Previously, changing those prompts created a different active window,
losing its focus and scroll state. A rendered-menu regression reproduced this
and now verifies the same window and retained scroll after a device switch.

Idle stick/trigger events no longer change the active device. Stick navigation
uses a deadzone, the left stick reaches ImGui navigation, and the right stick
scrolls the focused content area, including non-interactive controls text.
Returning to mouse/keyboard releases gamepad navigation and gamepad-owned active
widgets, rather than continuing to scroll or edit under the mouse. Real mouse
movement, clicks and wheel input select the mouse; zero-relative-motion events
do not impersonate deliberate mouse activity. SDL inverted-wheel direction and
wheel cursor coordinates are respected.

Additional nearby fixes: Ctrl+Shift+Tab now moves backward through tabs, and
focus loss clears input and the R3/L2 chord state without discarding settings.
The existing close/reopen lifetime isolation and both hotkey orders still pass.

`overlay-input-test` uses a virtual SDL controller and the real ImGui frame/menu
code at 640x480. It verifies overflow, normal/inverted wheel scroll (174 px),
right-stick scroll (116 px), device-switch identity/scroll retention, drift,
left-stick input, mouse takeover, reverse tabs and focus loss. No Vulkan or
game assets are needed. The full 48 C/C++ suites and 316 Python tests pass
(39 Python skips). This is automated event/layout coverage, not a physical
controller or final-swapchain screenshot sign-off.

## Runtime mouse sampling normalization

The actual `runtime_pad.c::mouse_input` path now accumulates counts over a
16 ms window and normalizes them to the existing 60 Hz response curve using
the pad's monotonic timestamps. It preserves the radial curve, anti-deadzone,
X/Y sensitivities, inversion, mouse buttons and controller addition. Losing
capture clears accumulated motion and the held deflection; stopping returns to
neutral after the movement-containing window and the next empty window.

`pad-map-test` drives the actual runtime with a stubbed mouse source. At a fixed
300 counts/s, the old path produced deflections 68 at 30 Hz and 48 at 60 Hz.
After the fix, 30/60/120/1000 Hz all produce 48. The test also checks rest,
capture loss, inversion, independent Y sensitivity and mouse fire. Assertions
in this Windows test report to stderr instead of opening blocking dialogs.
The full build, all 48 C/C++ suites and the Python suite pass.

This establishes synthetic sampling-rate normalization, not native 1:1 camera
look or measured in-game angular displacement. The existing 60 Hz curve is the
reference; no camera hook, turn-speed limit override or sensitivity-default
change was introduced. Gameplay tuning and #37 reporter acceptance remain open.

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

`src\runtime_pad.c::mouse_to_stick` still turns normalized mouse deltas into
bounded virtual-stick deflections with deadzone compensation and a nonlinear
magnitude curve. The caller now consumes elapsed time as described above.
Including `mouse_stick.h` does not connect that header's separate model to the
actual camera path; the new regression exercises the runtime itself. Neither
that test nor the standalone header tests establish native mouse behavior.

Do not call this 1:1/raw camera look or declare the 400% sensitivity issue fixed.
Needed evidence: repeatable gameplay camera displacement for identical raw counts
at 30/60/120 FPS, controlled polling rates, saturation tests and a safe engine-camera
integration seam. No engine hooks or guest-code patches were introduced here.

The broader Insert / R3+L2 keyboard/mouse/gamepad interaction remains subject to the
manual checklist in `MANUAL_GAMEPLAY_TESTS.md`; simultaneous physical input was
NOT TESTED in these automated frontend/prologue runs.
