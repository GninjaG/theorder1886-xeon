/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Mouse motion to right stick (src/mouse_stick.h): speed, smoothing, decay, anti-dead zone. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../src/mouse_stick.h"

static const float frame = 1.0f / 60.0f;

/* Moves the mouse at `speed` counts/s on x for `seconds`, sampled every `step`. */
static void move(MouseStick *s, const MouseStickParams *p, float speed, float seconds, float step) {
    for (float t = 0; t < seconds - 1e-6f; t += step) {
        mouse_stick_update(s, p, speed * step, 0, step);
    }
}

int main(void) {
    MouseStickParams p = {1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0};
    MouseStick s = {0};

    /* Linear: half the full speed is half a deflection; full speed and more clamp to 1. */
    move(&s, &p, MOUSE_STICK_FULL_SPEED * 0.5f, 0.1f, frame);
    assert(fabsf(mouse_stick_axis(&s, &p, 0) - 0.5f) < 0.01f);
    move(&s, &p, MOUSE_STICK_FULL_SPEED * 3.0f, 0.1f, frame);
    assert(mouse_stick_axis(&s, &p, 0) == 1.0f);
    move(&s, &p, -MOUSE_STICK_FULL_SPEED * 3.0f, 0.1f, frame);
    assert(mouse_stick_axis(&s, &p, 0) == -1.0f);

    /* Stopped: exactly 0 on the next window, no drift. */
    move(&s, &p, 0, 0.05f, frame);
    assert(mouse_stick_axis(&s, &p, 0) == 0.0f && s.out[0] == 0.0f);

    /* Sensitivity scales the speed. */
    p.sensitivity_x = 2.0f;
    move(&s, &p, MOUSE_STICK_FULL_SPEED * 0.25f, 0.1f, frame);
    assert(fabsf(mouse_stick_axis(&s, &p, 0) - 0.5f) < 0.01f);
    p.sensitivity_x = 1.0f;

    /* Pad sampled faster than the mouse reports (1 ms samples, a report every 8 ms): the 16 ms
     * window keeps the deflection steady instead of flickering to 0 between reports. */
    s = (MouseStick){0};
    float lo = 1, hi = 0;
    for (int ms = 0; ms < 200; ++ms) {
        const float dx = ms % 8 == 0 ? MOUSE_STICK_FULL_SPEED * 0.5f * 0.008f : 0.0f;
        mouse_stick_update(&s, &p, dx, 0, 0.001f);
        if (ms >= 40) {
            const float v = mouse_stick_axis(&s, &p, 0);
            lo = v < lo ? v : lo;
            hi = v > hi ? v : hi;
        }
    }
    assert(lo > 0.4f && hi < 0.6f);

    /* Acceleration: slow moves stay fine, fast ones go further. */
    p.accel = 1.0f;
    s = (MouseStick){0};
    move(&s, &p, MOUSE_STICK_FULL_SPEED * 0.1f, 0.1f, frame);
    assert(fabsf(mouse_stick_axis(&s, &p, 0) - 0.11f) < 0.01f);
    move(&s, &p, MOUSE_STICK_FULL_SPEED * 0.5f, 0.1f, frame);
    assert(fabsf(mouse_stick_axis(&s, &p, 0) - 0.75f) < 0.01f);
    p.accel = 0.0f;

    /* Anti-dead zone: the smallest motion is lifted past the game's dead zone; rest stays 0. */
    p.deadzone = 0.2f;
    s = (MouseStick){0};
    move(&s, &p, MOUSE_STICK_FULL_SPEED * 0.05f, 0.1f, frame);
    assert(fabsf(mouse_stick_axis(&s, &p, 0) - (0.2f + 0.8f * 0.05f)) < 0.01f);
    move(&s, &p, 0, 0.05f, frame);
    assert(mouse_stick_axis(&s, &p, 0) == 0.0f);

    /* Y: mouse down is stick down (bytes grow downwards); inverted flips it. */
    s = (MouseStick){0};
    p.deadzone = 0.0f;
    for (int i = 0; i < 6; ++i) mouse_stick_update(&s, &p, 0, MOUSE_STICK_FULL_SPEED * frame, frame);
    assert(mouse_stick_axis(&s, &p, 1) == 1.0f);
    p.invert_y = 1;
    for (int i = 0; i < 6; ++i) mouse_stick_update(&s, &p, 0, MOUSE_STICK_FULL_SPEED * frame, frame);
    assert(mouse_stick_axis(&s, &p, 1) == -1.0f);

    /* Bytes: centre 128, full left/up 0, full right/down 255, added to a base and clamped. */
    assert(mouse_stick_add(128, 0.0f) == 128 && mouse_stick_add(128, -1.0f) == 0 && mouse_stick_add(128, 1.0f) == 255);
    assert(mouse_stick_add(200, 1.0f) == 255 && mouse_stick_add(128, 0.5f) == 192);

    /* Huge or zero time steps are clamped (pauses, two reads in the same microsecond). */
    s = (MouseStick){0};
    mouse_stick_update(&s, &p, 1e6f, 0, 0.0f);
    mouse_stick_update(&s, &p, 0, 0, 10.0f);
    assert(s.out[0] == 0.0f || s.out[0] == 1.0f);

    puts("PASS: mouse stick speed, deadzone, accel, and inversion");
    return 0;
}
