# Input status and validation gaps

2026-10-22 source checkpoint; native camera input is NOT implemented by this work.

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
NOT TESTED in this automated title-menu checkpoint.
