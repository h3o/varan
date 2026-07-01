// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// keys.c — see keys.h. MPR121 cap-touch scan for the Pocket Varan.
//
// The register-init sequence is carried over from the prototype's
// keyboard.c::init_MPR121() (board-agnostic, just i2c writes) and relicensed
// MIT. Reads of the 2-byte touch-status register (0x00/0x01) are gated by the
// chip's IRQ line (PE14, active-low): the scan thread checks the PE14 input
// *level* straight from the PIO DATA register (mmap, no syscall) each tick and
// only touches i2c when the MPR121 is asserting a change, so bus-0 traffic is
// near zero at idle. (True edge interrupts aren't an option: the V3s only wires
// EINT controllers to ports B and G, not E — there is no PE_EINT / sysfs edge.)
// If /dev/mem can't be mapped we fall back to reading i2c every tick.

#include "hal/keys.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "hal/i2c.h"

// MPR121 touch/release thresholds (from the prototype's tuned VR29x values).
#define ELE_THRESHOLD_TOUCH   0x0B
#define ELE_THRESHOLD_RELEASE 0x06

#define KEYS_SCAN_INTERVAL_US 8000   // ~125 Hz level check (i2c only when gated)
#define KEYS_PAD_MASK         0x0FFF // ELE0..ELE11

// V3s PIO block: mmap the enclosing 4K page; registers are at GPIO_OFFSET 0x800
// with standard sunxi layout. The MPR121 IRQ is on PE14.
#define PIO_PHYS_PAGE   0x01C20000u
#define PIO_MAP_LEN     0x1000u
#define REG_WORD(off)   ((off) / 4u)
#define PE_CFG1_IDX     REG_WORD(0x800 + 0x94)  // PE pins 8..15 config
#define PE_DAT_IDX      REG_WORD(0x800 + 0xA0)  // PE input/output data
#define PE_PULL0_IDX    REG_WORD(0x800 + 0xAC)  // PE pins 0..15 pull
#define IRQ_PIN         14                      // PE14

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

// One atomic status read -> 12-bit bitmap. Returns 0 on success.
static int read_status(uint16_t *out) {
  uint8_t buf[2] = {0, 0};
  if (i2cget(I2C_CAP, 0x00, buf, 2) != 0) return 1;
  *out = ((uint16_t)buf[0] | ((uint16_t)buf[1] << 8)) & KEYS_PAD_MASK;
  return 0;
}

// Diff against prev and emit per-pad down/up events.
static void emit_changes(uint16_t now, uint16_t *prev) {
  touch_bitmap = now;
  uint16_t changed = now ^ *prev;
  if (!changed) return;
  for (int pad = 0; pad < KEYS_N_PADS; pad++) {
    uint16_t mask = (uint16_t)(1u << pad);
    if (changed & mask) {
      int pressed = (now & mask) ? 1 : 0;
      printf("keys: pad %d %s\n", pad, pressed ? "down" : "up");
      if (event_cb) event_cb(pad, pressed);
    }
  }
  *prev = now;
}

static void log_read_timeout(unsigned *err_total) {
  if (++(*err_total) == 1 || (*err_total % 128) == 0)
    fprintf(stderr, "keys: MPR121 read timeouts: %u\n", *err_total);
}

// --- PE14 IRQ line, read as a GPIO input level via the PIO registers --------
static volatile uint32_t *pio = 0;  // mapped PIO page, or NULL if unavailable

static int pio_map(void) {
  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) return 1;
  void *m = mmap(NULL, PIO_MAP_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, PIO_PHYS_PAGE);
  close(fd);
  if (m == MAP_FAILED) return 1;
  pio = (volatile uint32_t *)m;
  return 0;
}

// Configure PE14 as a GPIO input with the internal pull-up (IRQ is open-drain).
static void irq_pin_setup(void) {
  pio[PE_CFG1_IDX] &= ~(0xFu << ((IRQ_PIN - 8) * 4));  // 0b000 = input
  uint32_t v = pio[PE_PULL0_IDX];
  v &= ~(0x3u << (IRQ_PIN * 2));
  v |= (0x1u << (IRQ_PIN * 2));                          // 01 = pull-up
  pio[PE_PULL0_IDX] = v;
}

// Active-low: the MPR121 is asserting (data pending) when PE14 reads 0.
static int irq_asserted(void) {
  return ((pio[PE_DAT_IDX] >> IRQ_PIN) & 1u) == 0;
}

static void *keys_scan(void *arg) {
  (void)arg;
  uint16_t prev = 0;
  unsigned err_total = 0;

  int gated = (pio_map() == 0);
  if (gated) {
    irq_pin_setup();
    printf("keys: MPR121 IRQ-gated mode (PE14 level, i2c only on assert)\n");
  } else {
    printf("keys: MPR121 timed-poll mode (/dev/mem unavailable)\n");
  }

  // Prime once so the first real touch shows up as a change.
  {
    uint16_t now;
    if (read_status(&now) == 0) emit_changes(now, &prev);
  }

  while (scan_running) {
    // Only touch the bus when the chip is asserting (or if we can't gate).
    if (!gated || irq_asserted()) {
      uint16_t now;
      if (read_status(&now) == 0) emit_changes(now, &prev);
      else log_read_timeout(&err_total);
    }
    usleep(KEYS_SCAN_INTERVAL_US);
  }

  if (pio) {
    munmap((void *)pio, PIO_MAP_LEN);
    pio = 0;
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
