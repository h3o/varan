// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// varan_ui.h — navigate-only UI for the Pocket Varan player skeleton.
//
// Phase 0: this is the prototype's Varan_UI lifted OFF the synth framework.
// The original inherited Patches -> Engines (dsp/Engines.h), which transitively
// pulled in every DSP engine. Here it inherits nothing: it just drives the OLED
// file browser from the GPIO buttons and the cap-touch keys. Playback wiring
// (the audio engine + command socket) lands in Phase 1.

#ifndef VARAN_UI_H_
#define VARAN_UI_H_

#include <cstdint>

// Forward declarations keep the synth-free header dependency-light; the real
// classes are migrated into firmware/hal (Display) and firmware/app (browser).
class Display;
class FileSystemBrowser;

#define VARAN_N_BUTTONS  5
#define VARAN_LOOP_US    50000  // 50 ms UI tick

class VaranUI {
 public:
  VaranUI();
  ~VaranUI();

  // Blocks, running the navigate-only loop until exit_program is set.
  void run();

 private:
  int  init_buttons();
  void read_buttons();

  Display          *oled_    = nullptr;
  FileSystemBrowser *browser_ = nullptr;

  int      btn_fd_[VARAN_N_BUTTONS];   // open sysfs gpio value fds
  int      btn_val_[VARAN_N_BUTTONS];  // last read state, 1 = pressed
  uint32_t loop_counter_ = 0;
};

#endif  // VARAN_UI_H_
