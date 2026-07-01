// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// main.cpp — Pocket Varan entry point.
//
// Linear bring-up: LEDs, cap-touch keys, MIDI, the audio engine + command
// listener (Phase 1), then the navigate-only UI loop. No board #ifdef soup, no
// TEST_SOUND_ENGINE, no synth.

#include <csignal>
#include <cstdio>

#include "hal/hal.h"
#include "app/varan_ui.h"
#include "audio/engine.h"
#include "audio/cmd_listener.h"

// Set by SIGINT; the UI loop and audio threads watch it to shut down cleanly.
volatile sig_atomic_t exit_program = 0;

static void on_sigint(int sig) {
  fprintf(stderr, "\nmain: caught signal %d, exiting\n", sig);
  exit_program = 1;
}

int main(int /*argc*/, char ** /*argv*/) {
  signal(SIGINT, on_sigint);
  signal(SIGPIPE, SIG_IGN);  // don't die if a control-socket client vanishes

  hal_leds_init();
  hal_leds_startup_animation();

  if (hal_keys_init() != 0) {
    fprintf(stderr, "main: hal_keys_init() failed\n");
    // Non-fatal: navigation still works from the GPIO buttons.
  }

  hal_midi_init();

  // Audio engine + its command surface (FIFO + Unix socket).
  engine_init();
  cmd_listener_start();

  VaranUI ui;
  ui.run();  // blocks until exit_program (or a QUIT command)

  cmd_listener_stop();
  engine_shutdown();
  hal_leds_all_off();
  return 0;
}
