// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// varan_ui.cpp — navigate-only UI loop (Phase 0).
//
// Logic is preserved from the prototype's Varan_UI::VARAN(): open the GPIO
// button value files, start the cap-touch key driver, then poll buttons every
// tick and drive the OLED file browser. What's gone is the Patches/Engines
// inheritance and every synth include.

#include "app/varan_ui.h"

#include <csignal>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

#include "hal/hal.h"
// Migrated alongside this skeleton (see firmware/MIGRATION.md):
#include "hal/display.h"   // class Display  (OLED, ex hw/Display.h)
#include "app/browser.h"   // class FileSystemBrowser (ex hw/SDcard_browser.h)

// Defined in main.cpp; set by the SIGINT handler.
extern volatile sig_atomic_t exit_program;

// Current "v2" button wiring (see linux/root/read_buttons_v2.sh and
// docs/hardware.md). Index order matches the browser action mapping in run().
static const struct {
  int gpio;
  const char *role;
} kButtons[VARAN_N_BUTTONS] = {
    {35,  "power"},       // PB3
    {147, "center"},      // PE19
    {151, "top-middle"},  // PE23
    {34,  "left"},        // PB2
    {193, "right"},       // PG1
};

VaranUI::VaranUI() {
  printf("VaranUI: constructing\n");
  oled_    = new Display();
  browser_ = new FileSystemBrowser(oled_, "/mnt/SD");
  for (int i = 0; i < VARAN_N_BUTTONS; i++) {
    btn_fd_[i]  = -1;
    btn_val_[i] = 0;
  }
}

VaranUI::~VaranUI() {
  printf("VaranUI: destructing\n");
  delete browser_;
  delete oled_;
  hal_i2c_deinit();
  for (int i = 0; i < VARAN_N_BUTTONS; i++) {
    if (btn_fd_[i] >= 0) close(btn_fd_[i]);
  }
  hal_keys_stop();
}

int VaranUI::init_buttons() {
  for (int i = 0; i < VARAN_N_BUTTONS; i++) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", kButtons[i].gpio);
    btn_fd_[i] = open(path, O_RDONLY);
    if (btn_fd_[i] < 0) {
      printf("VaranUI: cannot open %s (%s button)\n", path, kButtons[i].role);
      return i + 1;
    }
  }
  return 0;
}

void VaranUI::read_buttons() {
  for (int i = 0; i < VARAN_N_BUTTONS; i++) {
    char c = '1';
    lseek(btn_fd_[i], 0, SEEK_SET);
    if (read(btn_fd_[i], &c, 1) != 1) c = '1';
    // The center button reads active-high; the rest are active-low.
    btn_val_[i] = (i == 1) ? (c - '0') : ('1' - c);
  }
}

void VaranUI::run() {
  printf("VaranUI: run()\n");

  if (init_buttons()) {
    printf("VaranUI: button init failed, aborting\n");
    exit_program = 1;
    return;
  }

  hal_keys_start();

  while (!exit_program) {
    loop_counter_++;
    read_buttons();

    // 1=up, 2=down, 3=left, 4=right
    if (btn_val_[2]) browser_->updateButtons(1);  // top-middle
    if (btn_val_[1]) browser_->updateButtons(2);  // center
    if (btn_val_[3]) browser_->updateButtons(3);  // left
    if (btn_val_[4]) browser_->updateButtons(4);  // right
    // btn_val_[0] (power) is reserved for the soft-shutdown handler.

    browser_->update();
    usleep(VARAN_LOOP_US);
  }
}
