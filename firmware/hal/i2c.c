// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// i2c.c — see i2c.h. Adapted from the prototype's peripherals.c i2c helpers
// (themselves after https://github.com/alexportnov/i2c_raw), trimmed to this
// board and relicensed MIT.

#include "hal/i2c.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define I2C_UNALLOCATED (-1)

int i2c_fd[I2C_DEVICES] = {I2C_UNALLOCATED, I2C_UNALLOCATED, I2C_UNALLOCATED};

static int i2c_open(int bus, int addr) {
  char path[64];
  snprintf(path, sizeof(path), "/dev/i2c-%d", bus);

  int fd = open(path, O_RDWR);
  if (fd < 0) {
    fprintf(stderr, "i2c_open: cannot open %s (errno %d)\n", path, errno);
    return -1;
  }

  unsigned long funcs;
  if (ioctl(fd, I2C_FUNCS, &funcs) < 0) {
    fprintf(stderr, "i2c_open: I2C_FUNCS error %d\n", errno);
    close(fd);
    return -1;
  }
  if ((funcs & I2C_FUNC_I2C) == 0) {
    fprintf(stderr, "i2c_open: %s is SMBus-only, not I2C\n", path);
    close(fd);
    return -1;
  }
  if (ioctl(fd, I2C_SLAVE_FORCE, addr) < 0) {
    fprintf(stderr, "i2c_open: I2C_SLAVE_FORCE 0x%02x error %d\n", addr, errno);
    close(fd);
    return -1;
  }
  return fd;
}

int i2c_init(void) {
  if (i2c_fd[I2C_ACC] == I2C_UNALLOCATED) {
    i2c_fd[I2C_ACC] = i2c_open(I2C_BUS, ACC_I2C_ADDR);
    if (i2c_fd[I2C_ACC] < 0) return 1;
  }
  if (i2c_fd[I2C_CAP] == I2C_UNALLOCATED) {
    i2c_fd[I2C_CAP] = i2c_open(I2C_BUS, CAP_I2C_ADDR);
    if (i2c_fd[I2C_CAP] < 0) return 2;
  }
  if (i2c_fd[I2C_OLED] == I2C_UNALLOCATED) {
    i2c_fd[I2C_OLED] = i2c_open(I2C_BUS_OLED, OLED_I2C_ADDRESS);
    if (i2c_fd[I2C_OLED] < 0) return 3;
  }
  return 0;
}

int i2c_deinit(void) {
  for (int i = 0; i < I2C_DEVICES; i++) {
    if (i2c_fd[i] > I2C_UNALLOCATED) {
      close(i2c_fd[i]);
      i2c_fd[i] = I2C_UNALLOCATED;
    }
  }
  return 0;
}

static int i2c_valid(int device) {
  return device >= 0 && device < I2C_DEVICES && i2c_fd[device] > I2C_UNALLOCATED;
}

int i2cset(int device, uint8_t reg, uint8_t val) {
  if (!i2c_valid(device)) { fprintf(stderr, "i2cset: bad device %d\n", device); return 1; }
  uint8_t data[2] = {reg, val};
  if (write(i2c_fd[device], data, 2) != 2) {
    fprintf(stderr, "i2cset: write error %d\n", errno);
    return 2;
  }
  return 0;
}

int i2cset_w(int device, uint8_t reg, uint8_t val1, uint8_t val2) {
  if (!i2c_valid(device)) { fprintf(stderr, "i2cset_w: bad device %d\n", device); return 1; }
  uint8_t data[3] = {reg, val1, val2};
  if (write(i2c_fd[device], data, 3) != 3) {
    fprintf(stderr, "i2cset_w: write error %d\n", errno);
    return 2;
  }
  return 0;
}

static uint8_t i2c_addr(int device) {
  switch (device) {
    case I2C_ACC:  return ACC_I2C_ADDR;
    case I2C_CAP:  return CAP_I2C_ADDR;
    case I2C_OLED: return OLED_I2C_ADDRESS;
  }
  return 0;
}

int i2cget(int device, uint8_t reg, uint8_t *val, int bytes) {
  if (!i2c_valid(device)) return 1;
  // One atomic transaction (repeated START, no STOP between the register write
  // and the read). Doing write()+read() as two transactions can wedge the
  // sunxi/mv64xxx controller ("I2C bus locked") under load, so use I2C_RDWR.
  struct i2c_msg msgs[2];
  msgs[0].addr  = i2c_addr(device);
  msgs[0].flags = 0;
  msgs[0].len   = 1;
  msgs[0].buf   = &reg;
  msgs[1].addr  = i2c_addr(device);
  msgs[1].flags = I2C_M_RD;
  msgs[1].len   = (uint16_t)bytes;
  msgs[1].buf   = val;
  struct i2c_rdwr_ioctl_data xfer = { msgs, 2 };
  if (ioctl(i2c_fd[device], I2C_RDWR, &xfer) < 0) {
    return 3;  // caller decides whether/how to log (kept quiet to avoid spam)
  }
  return 0;
}

int i2cfill(int device, uint8_t reg, uint8_t value, int length) {
  if (!i2c_valid(device)) { fprintf(stderr, "i2cfill: bad device %d\n", device); return 1; }
  if (length < 0) return 1;
  uint8_t data[length + 1];
  data[0] = reg;
  memset(data + 1, value, length);
  if (write(i2c_fd[device], data, length + 1) != length + 1) {
    fprintf(stderr, "i2cfill: write error %d\n", errno);
    return 2;
  }
  return 0;
}

int i2cwrite_columns(int device, uint8_t reg, uint8_t *data, int length, int interleave) {
  if (!i2c_valid(device)) { fprintf(stderr, "i2cwrite_columns: bad device %d\n", device); return 1; }
  if (length < 0) return 1;
  uint8_t out[length + 1];
  out[0] = reg;
  for (int col = 0; col < length; col++) {
#ifdef SSD1306_ROTATE
    out[length - col] = data[col * interleave];
#else
    out[col + 1] = data[col * interleave];
#endif
  }
  if (write(i2c_fd[device], out, length + 1) != length + 1) {
    fprintf(stderr, "i2cwrite_columns: write error %d\n", errno);
    return 2;
  }
  return 0;
}
