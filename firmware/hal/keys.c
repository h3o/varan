// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// keys.c — see keys.h. MPR121 cap-touch scan for the Pocket Varan.
//
// The register-init sequence is carried over from the prototype's
// keyboard.c::init_MPR121() (board-agnostic, just i2c writes) and relicensed
// MIT. Reads of the 2-byte touch-status register (0x00/0x01) are gated by the
// chip's IRQ line (PE14/gpio142, active-low): the scan thread blocks in poll()
// on the sysfs GPIO edge and only touches i2c when the MPR121 signals a change,
// so bus-0 traffic (and CPU) are near zero at idle. If the IRQ GPIO can't be set
// up we fall back to a plain timed poll.

#include "hal/keys.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "hal/i2c.h"

// MPR121 touch/release thresholds (from the prototype's tuned VR29x values).
#define ELE_THRESHOLD_TOUCH   0x0B
#define ELE_THRESHOLD_RELEASE 0x06

#define KEYS_SCAN_INTERVAL_US 16000  // timed-poll fallback period (~60 Hz)
#define KEYS_IRQ_TIMEOUT_MS   500    // IRQ mode: safety-net re-read if no edge
#define KEYS_PAD_MASK         0x0FFF // ELE0..ELE11

#define MPR121_IRQ_GPIO   142  // PE14, active-low, internal pull-up
#define MPR121_IRQ_VALUE  "/sys/class/gpio/gpio142/value"
#define MPR121_IRQ_EDGE   "/sys/class/gpio/gpio142/edge"
#define MPR121_IRQ_DIR    "/sys/class/gpio/gpio142/direction"
// PIO page + PE_PULL0 register offset, for the internal pull-up on PE14.
#define PIO_PHYS_PAGE     0x01C20000u
#define PE_PULL0_OFFSET   0x8ACu

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

static void sysfs_write(const char *path, const char *val) {
  int fd = open(path, O_WRONLY);
  if (fd >= 0) {
    (void)!write(fd, val, strlen(val));
    close(fd);
  }
}

// Enable the PE14 internal pull-up (the IRQ is open-drain). Best-effort.
static void enable_irq_pullup(void) {
  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) return;
  void *m = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, PIO_PHYS_PAGE);
  close(fd);
  if (m == MAP_FAILED) return;
  volatile uint32_t *pull0 = (volatile uint32_t *)((char *)m + PE_PULL0_OFFSET);
  uint32_t v = *pull0;
  v &= ~(0x3u << 28);  // PE14 pull bits [29:28]
  v |= (0x1u << 28);   // 01 = pull-up
  *pull0 = v;
  munmap(m, 0x1000);
}

static int edge_is_falling(void) {
  int fd = open(MPR121_IRQ_EDGE, O_RDONLY);
  if (fd < 0) return 0;
  char b[16] = {0};
  ssize_t n = read(fd, b, sizeof(b) - 1);
  close(fd);
  return n > 0 && strncmp(b, "falling", 7) == 0;
}

// Set the IRQ pin up as a falling-edge input with pull-up. Returns an open read
// fd on the value file (*edge_ok set if edge interrupts are usable), or -1 if
// the GPIO isn't available at all.
static int configure_irq_gpio(int *edge_ok) {
  sysfs_write("/sys/class/gpio/export", "142");  // ignored if already exported
  sysfs_write(MPR121_IRQ_DIR, "in");
  sysfs_write(MPR121_IRQ_EDGE, "falling");
  enable_irq_pullup();
  *edge_ok = edge_is_falling();
  return open(MPR121_IRQ_VALUE, O_RDONLY);
}

static void log_read_timeout(unsigned *err_total) {
  if (++(*err_total) == 1 || (*err_total % 128) == 0)
    fprintf(stderr, "keys: MPR121 read timeouts: %u\n", *err_total);
}

static void *keys_scan(void *arg) {
  (void)arg;
  uint16_t prev = 0;
  unsigned err_total = 0;

  int edge_ok = 0;
  int irq_fd = configure_irq_gpio(&edge_ok);

  if (irq_fd >= 0 && edge_ok) {
    printf("keys: MPR121 IRQ-gated mode (gpio%d, falling edge)\n", MPR121_IRQ_GPIO);
    struct pollfd pfd;
    pfd.fd = irq_fd;
    pfd.events = POLLPRI | POLLERR;

    // Consume the initial level and clear any pending IRQ before waiting.
    char c;
    lseek(irq_fd, 0, SEEK_SET);
    (void)!read(irq_fd, &c, 1);
    {
      uint16_t now;
      if (read_status(&now) == 0) emit_changes(now, &prev);
    }

    while (scan_running) {
      int r = poll(&pfd, 1, KEYS_IRQ_TIMEOUT_MS);
      if (!scan_running) break;
      if (r < 0) {
        if (errno == EINTR) continue;
        break;
      }
      if (r > 0) {  // edge fired: re-read the value to rearm the poll
        lseek(irq_fd, 0, SEEK_SET);
        (void)!read(irq_fd, &c, 1);
      }
      // On an edge (r>0) or the safety-net timeout (r==0): read the MPR121
      // status (which de-asserts its IRQ) and emit any changes.
      uint16_t now;
      if (read_status(&now) == 0) emit_changes(now, &prev);
      else log_read_timeout(&err_total);
    }
  } else {
    printf("keys: MPR121 timed-poll mode (gpio%d edge unavailable)\n", MPR121_IRQ_GPIO);
    if (irq_fd >= 0) { close(irq_fd); irq_fd = -1; }
    while (scan_running) {
      uint16_t now;
      if (read_status(&now) == 0) emit_changes(now, &prev);
      else log_read_timeout(&err_total);
      usleep(KEYS_SCAN_INTERVAL_US);
    }
  }

  if (irq_fd >= 0) close(irq_fd);
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
