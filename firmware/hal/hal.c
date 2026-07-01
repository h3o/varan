// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// hal.c — implementations behind hal.h.
//
// Phase 0 scope: i2c, the MPR121 cap-touch keyboard, and the LEDs are real.
// MIDI is still stubbed — it's the Phase 1 control surface, not the prototype's
// synth note parser. See firmware/MIGRATION.md.

#include "hal/hal.h"

#include <stdio.h>

#include "hal/i2c.h"
#include "hal/keys.h"
#include "hal/leds.h"

// --- LEDs ------------------------------------------------------------------
// Real: direct-register driver over /dev/mem (hal/leds.{c,h}). Pin directions
// come from linux/root/all_LEDs_init.sh at boot; this only drives DATA bits.
void hal_leds_init(void)              { leds_init(); }
void hal_leds_startup_animation(void) { leds_startup_animation(); }
void hal_leds_all_off(void)           { leds_all_off(); }

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
