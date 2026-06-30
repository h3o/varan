// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// main.cpp — Pocket Varan navigate-only entry point (Phase 0).
//
// Replaces the prototype's multi-board, engine-selecting main.cpp with a single
// linear bring-up: LEDs, cap-touch keys, MIDI, then the navigate-only UI loop.
// No board #ifdef soup, no TEST_SOUND_ENGINE, no synth. Phase 1 adds the audio
// engine and the command socket.

#include <csignal>
#include <cstdio>

#include "hal/hal.h"
#include "app/varan_ui.h"

// Set by SIGINT; the UI loop and audio threads watch it to shut down cleanly.
volatile sig_atomic_t exit_program = 0;

static void on_sigint(int sig) {
  fprintf(stderr, "\nmain: caught signal %d, exiting\n", sig);
  exit_program = 1;
}

int main(int /*argc*/, char ** /*argv*/) {
  signal(SIGINT, on_sigint);

  hal_leds_init();
  hal_leds_startup_animation();

  if (hal_keys_init() != 0) {
    fprintf(stderr, "main: hal_keys_init() failed\n");
    // Non-fatal: navigation still works from the GPIO buttons.
  }

  hal_midi_init();

  VaranUI ui;
  ui.run();  // blocks until exit_program

  hal_leds_all_off();
  return 0;
}
