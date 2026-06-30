// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// hal.c — implementations behind hal.h.
//
// Phase 0 scope: i2c is real (the OLED needs it); LEDs, cap-touch keys and MIDI
// are stubbed so the navigate-only build links and runs from the GPIO buttons
// alone. Each stub maps to the prototype function that will be migrated next
// (see firmware/MIGRATION.md): leds.c, keyboard.c (MPR121), midi.c.

#include "hal/hal.h"

#include <stdio.h>

#include "hal/i2c.h"

// --- LEDs ------------------------------------------------------------------
void hal_leds_init(void)              { /* TODO: migrate leds.c (init_leds_gingko) */ }
void hal_leds_startup_animation(void) { /* TODO: startup_leds_animation_gingko */ }
void hal_leds_all_off(void)           { /* TODO: LEDs_all_OFF */ }

// --- Cap-touch keyboard (MPR121) ------------------------------------------
int  hal_keys_init(void) {
  // TODO: migrate keyboard.c (init_keyboard_ginkgo_MPR121). Until then the UI
  // is driven by the GPIO buttons only; report success so bring-up continues.
  printf("hal_keys_init: stub (GPIO buttons only for now)\n");
  return 0;
}
void hal_keys_start(void) { /* TODO: keys_driver_VARAN */ }
void hal_keys_stop(void)  { /* TODO: keys_driver_VARAN_stop */ }

// --- MIDI ------------------------------------------------------------------
int  hal_midi_init(void) {
  // TODO: migrate midi.c (MIDI_driver_init). MIDI is the Phase 2 control surface.
  printf("hal_midi_init: stub\n");
  return 0;
}

// --- I2C (real) ------------------------------------------------------------
int  hal_i2c_init(void)   { return i2c_init(); }
void hal_i2c_deinit(void) { i2c_deinit(); }
