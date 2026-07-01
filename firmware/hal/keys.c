// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// keys.c — see keys.h. MPR121 cap-touch scan for the Pocket Varan.
//
// The register-init sequence is carried over from the prototype's
// keyboard.c::init_MPR121() (board-agnostic, just i2c writes) and relicensed
// MIT. The scan strategy is intentionally simple: poll the 2-byte touch-status
// register (0x00/0x01) on a short timer and diff the 12-bit bitmap. The chip's
// IRQ line (PE14/gpio142) could gate the i2c reads to shave CPU later, but a
// timed poll needs no exported GPIO and works regardless of board wiring.

#include "hal/keys.h"

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "hal/i2c.h"

// MPR121 touch/release thresholds (from the prototype's tuned VR29x values).
#define ELE_THRESHOLD_TOUCH   0x0B
#define ELE_THRESHOLD_RELEASE 0x06

#define KEYS_SCAN_INTERVAL_US 16000  // ~60 Hz status poll (plenty for touch)
#define KEYS_PAD_MASK         0x0FFF // ELE0..ELE11

static pthread_t      scan_thread;
static volatile int   scan_running = 0;
static keys_event_cb  event_cb = 0;
static volatile uint16_t touch_bitmap = 0;

// Program the MPR121 baseline-tracking filters, per-electrode thresholds and
// finally enable all 12 electrodes. Mirrors the datasheet-recommended sequence.
static int mpr121_configure(int dev) {
  if (i2cset(dev, 0x80, 0x63)) return 1;  // soft reset
  usleep(10000);

  if (i2cset(dev, 0x5e, 0x00)) return 2;  // stop mode (all electrodes off) while configuring

  // Rising / falling baseline filter (MHD, NHD, NCL, FDL).
  if (i2cset(dev, 0x2b, 0x01)) return 2;
  if (i2cset(dev, 0x2c, 0x01)) return 2;
  if (i2cset(dev, 0x2d, 0x00)) return 2;
  if (i2cset(dev, 0x2e, 0x00)) return 2;
  if (i2cset(dev, 0x2f, 0x01)) return 2;
  if (i2cset(dev, 0x30, 0x01)) return 2;
  if (i2cset(dev, 0x31, 0xff)) return 2;
  if (i2cset(dev, 0x32, 0x02)) return 2;

  // Touch/release thresholds for ELE0..ELE11 (registers 0x41..0x58, T then R).
  for (int ele = 0; ele < KEYS_N_PADS; ele++) {
    if (i2cset(dev, 0x41 + ele * 2, ELE_THRESHOLD_TOUCH))   return 2;
    if (i2cset(dev, 0x42 + ele * 2, ELE_THRESHOLD_RELEASE)) return 2;
  }

  if (i2cset(dev, 0x5c, 0x10)) return 2;  // FFI / CDC (defaults)
  if (i2cset(dev, 0x5d, 0x04)) return 2;  // CDT / SFI / ESI (16 ms sample interval)

  if (i2cset(dev, 0x5e, 0x0c)) return 2;  // run mode: enable ELE0..ELE11
  return 0;
}

int keys_init(void) {
  // Idempotent: keys_init() may run before the Display opens the buses.
  if (i2c_init()) {
    fprintf(stderr, "keys_init: i2c_init failed\n");
    return 1;
  }
  int res = mpr121_configure(I2C_CAP);
  if (res) {
    fprintf(stderr, "keys_init: MPR121 configure failed (%d)\n", res);
    return res;
  }
  printf("keys_init: MPR121 ready (%d pads on i2c0 @0x%02x)\n", KEYS_N_PADS, CAP_I2C_ADDR);
  return 0;
}

static void *keys_scan(void *arg) {
  (void)arg;
  uint16_t prev = 0;
  unsigned err_total = 0;

  while (scan_running) {
    uint8_t buf[2] = {0, 0};
    if (i2cget(I2C_CAP, 0x00, buf, 2) == 0) {
      uint16_t now = ((uint16_t)buf[0] | ((uint16_t)buf[1] << 8)) & KEYS_PAD_MASK;
      touch_bitmap = now;

      uint16_t changed = now ^ prev;
      if (changed) {
        for (int pad = 0; pad < KEYS_N_PADS; pad++) {
          uint16_t mask = (uint16_t)(1u << pad);
          if (changed & mask) {
            int pressed = (now & mask) ? 1 : 0;
            printf("keys: pad %d %s\n", pad, pressed ? "down" : "up");
            if (event_cb) event_cb(pad, pressed);
          }
        }
        prev = now;
      }
    } else {
      // Transient bus timeout/lock: skip this cycle, log sparsely.
      if (++err_total == 1 || (err_total % 128) == 0)
        fprintf(stderr, "keys: MPR121 read timeouts: %u\n", err_total);
    }
    usleep(KEYS_SCAN_INTERVAL_US);
  }
  return NULL;
}

void keys_start(keys_event_cb cb) {
  if (scan_running) return;
  event_cb = cb;
  scan_running = 1;
  if (pthread_create(&scan_thread, NULL, keys_scan, NULL) != 0) {
    fprintf(stderr, "keys_start: cannot create scan thread\n");
    scan_running = 0;
  }
}

void keys_stop(void) {
  if (!scan_running) return;
  scan_running = 0;
  pthread_join(scan_thread, NULL);
}

uint16_t keys_touch_state(void) {
  return touch_bitmap;
}
