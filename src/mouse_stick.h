/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Mouse motion as a right stick (runtime_pad.c): converts relative mouse movement
 * into smooth, responsive stick deflection with game deadzone compensation.
 * Features:
 *   1. Accumulation window (MOUSE_STICK_WINDOW_S = 0.016s) calculates physical mouse velocity
 *      independent of game framerate or input polling frequency.
 *   2. Independent horizontal (X) and vertical (Y) sensitivity scaling.
 *   3. Adjustable acceleration curve for fine aiming and swift turns.
 *   4. Zero-drift rest state when mouse motion ceases.
 *   5. Anti-deadzone bypass raising motion immediately past the engine's ~24.0/128 deadzone threshold.
 */
#ifndef THEORDER_MOUSE_STICK_H
#define THEORDER_MOUSE_STICK_H
#include <math.h>
#include <stdint.h>

#define MOUSE_STICK_FULL_SPEED 2400.0f /* counts/s for full deflection at sensitivity 1.0 */
#define MOUSE_STICK_WINDOW_S   0.016f  /* 16.6ms window for velocity integration */
#define MOUSE_STICK_REST       0.005f

typedef struct {
    float sensitivity_x; /* 0.05 .. 10.0 */
    float sensitivity_y; /* 0.05 .. 10.0 */
    float smoothing;     /* 0.0 (raw) .. 0.95 */
    float accel;         /* 0.0 .. 2.0 */
    float deadzone;      /* 0.0 .. 0.5 (default ~0.19 for The Order: 1886) */
    int invert_y;
} MouseStickParams;

typedef struct {
    float acc[2], acc_t;
    float target[2];
    float out[2];
} MouseStick;

static inline float mouse_stick_clamp(float v, float lo, float hi) {
    return v < lo ? lo : v > hi ? hi : v;
}

static inline void mouse_stick_update(MouseStick *s, const MouseStickParams *p, float dx, float dy, float dt) {
    if (dt > 0.1f) {
        dx *= 0.1f / dt;
        dy *= 0.1f / dt;
    }
    dt = mouse_stick_clamp(dt, 0.0005f, 0.1f);
    s->acc[0] += dx;
    s->acc[1] += p->invert_y ? -dy : dy;
    s->acc_t += dt;

    if (s->acc_t >= MOUSE_STICK_WINDOW_S - 1e-5f) {
        const float sens_x = mouse_stick_clamp(p->sensitivity_x, 0.05f, 10.0f);
        const float sens_y = mouse_stick_clamp(p->sensitivity_y > 0.0f ? p->sensitivity_y : p->sensitivity_x, 0.05f, 10.0f);
        const float accel = mouse_stick_clamp(p->accel, 0.0f, 2.0f);

        /* X axis */
        float tx = s->acc[0] / s->acc_t * sens_x / MOUSE_STICK_FULL_SPEED;
        tx *= 1.0f + accel * fabsf(tx);
        s->target[0] = mouse_stick_clamp(tx, -1.0f, 1.0f);
        s->acc[0] = 0.0f;

        /* Y axis */
        float ty = s->acc[1] / s->acc_t * sens_y / MOUSE_STICK_FULL_SPEED;
        ty *= 1.0f + accel * fabsf(ty);
        s->target[1] = mouse_stick_clamp(ty, -1.0f, 1.0f);
        s->acc[1] = 0.0f;

        s->acc_t = 0.0f;
    }

    const float smooth = mouse_stick_clamp(p->smoothing, 0.0f, 0.95f);
    const float k = smooth <= 0.0f ? 1.0f : 1.0f - powf(smooth, dt * 60.0f);
    for (int i = 0; i < 2; ++i) {
        s->out[i] += (s->target[i] - s->out[i]) * k;
        if (s->target[i] == 0.0f && fabsf(s->out[i]) < MOUSE_STICK_REST) {
            s->out[i] = 0.0f;
        }
    }
}

static inline float mouse_stick_axis(const MouseStick *s, const MouseStickParams *p, int axis) {
    const float v = s->out[axis], a = fabsf(v);
    if (a < MOUSE_STICK_REST) {
        return 0.0f;
    }
    const float dz = mouse_stick_clamp(p->deadzone, 0.0f, 0.5f);
    const float lifted = mouse_stick_clamp(dz + (1.0f - dz) * a, 0.0f, 1.0f);
    return v < 0 ? -lifted : lifted;
}

static inline uint8_t mouse_stick_add(uint8_t base, float deflection) {
    if (deflection == 0.0f) {
        return base;
    }
    const float v = (float)base + deflection * (deflection < 0 ? 128.0f : 127.0f);
    return (uint8_t)mouse_stick_clamp(roundf(v), 0.0f, 255.0f);
}

#endif
