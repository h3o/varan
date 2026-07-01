// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// hal.c — implementations behind hal.h.
//
// Phase 0 scope: i2c and the MPR121 cap-touch keyboard are real; LEDs and MIDI
// are still stubbed. LEDs are blocked on the board's LED wiring (the prototype
// never implemented a VR29x LED driver — only the RDA8810 "gingko" one); MIDI
// is the Phase 1 control surface. See firmware/MIGRATION.md.

#include "hal/hal.h"

#include <stdio.h>

#include "hal/i2c.h"
#include "hal/keys.h"

// --- LEDs ------------------------------------------------------------------
// No-ops for now: the prototype has no VR29x LED driver to migrate, and the
// LED matrix wiring (leds.h: 23 LEDs, 2x9 + blue) lives in the board repo.
void hal_leds_init(void)              { /* TODO: needs VR29x LED wiring */ }
void hal_leds_startup_animation(void) { /* TODO: needs VR29x LED wiring */ }
void hal_leds_all_off(void)           { /* TODO: needs VR29x LED wiring */ }

// --- Cap-touch keyboard (MPR121) ------------------------------------------
static void hal_keys_log_cb(int pad, int pressed) {
  // Phase 0: just observe. Phase 1 routes pads to transport/loop/FX actions.
  (void)pad;
  (void)pressed;
}

int  hal_keys_init(void) { return keys_init(); }
void hal_keys_start(void) { keys_start(hal_keys_log_cb); }
void hal_keys_stop(void)  { keys_stop(); }

// --- MIDI ------------------------------------------------------------------
int  hal_midi_init(void) {
  // Deferred: MIDI-CC playback control is the Phase 1 control surface, not the
  // prototype's synth note parser (midi.c).
  printf("hal_midi_init: deferred to Phase 1\n");
  return 0;
}

// --- I2C (real) ------------------------------------------------------------
int  hal_i2c_init(void)   { return i2c_init(); }
void hal_i2c_deinit(void) { i2c_deinit(); }
