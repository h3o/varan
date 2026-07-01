// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// keys.h — cap-touch keyboard driver for the Pocket Varan (MPR121 on i2c0 @0x5a).
//
// A clean, minimal rewrite. The prototype's keyboard.c had a working MPR121
// register-init sequence for this board, but its event path (indicate_keyboard)
// was an empty stub on VR29x and everything around it was tangled into synth
// aftertouch/velocity/MIDI-note/menu machinery. None of that belongs in a media
// player, so this driver keeps only what a player needs: configure the chip,
// scan the 12 electrodes, and report a pad going down/up. What each pad *means*
// (transport, loop points, FX) is decided later by the app / command surface.

#ifndef VARAN_HAL_KEYS_H_
#define VARAN_HAL_KEYS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// MPR121 electrodes wired as touch pads on the Varan (ELE0..ELE11).
#define KEYS_N_PADS 12

// Delivered from the scan thread when a pad's state changes.
// pad = 0..KEYS_N_PADS-1; pressed = 1 on touch, 0 on release.
typedef void (*keys_event_cb)(int pad, int pressed);

// Configure the MPR121 for 12-electrode touch sensing. Opens the i2c buses if
// they aren't already (idempotent). Returns 0 on success, non-zero on i2c
// failure (e.g. the chip isn't present) — callers should treat that as
// non-fatal and fall back to the GPIO buttons.
int keys_init(void);

// Launch the background scan thread. Events go to cb (pass NULL to only update
// the latched bitmap). A second call while running is a no-op.
void keys_start(keys_event_cb cb);

// Stop and join the scan thread. Safe to call if never started.
void keys_stop(void);

// Latest 12-bit touch bitmap (bit i set = pad i currently touched).
uint16_t keys_touch_state(void);

#ifdef __cplusplus
}
#endif

#endif  // VARAN_HAL_KEYS_H_
