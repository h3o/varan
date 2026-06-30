// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// i2c.h — minimal I2C layer for the Pocket Varan (VR294 / Allwinner V3s).
//
// A clean re-implementation of the i2c primitives the prototype kept in its
// board-#ifdef'd peripherals.c. Only what this board needs: cap-touch and
// accelerometer on /dev/i2c-0, the OLED on /dev/i2c-1. Same wire behaviour as
// the prototype (raw write of [reg, data...] to a slave selected with
// I2C_SLAVE_FORCE), so the ssd1306 driver works unchanged.

#ifndef VARAN_HAL_I2C_H_
#define VARAN_HAL_I2C_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Logical device handles (index into the open-fd table).
#define I2C_ACC      0   // LIS3DH accelerometer  -> /dev/i2c-0 @ 0x19
#define I2C_CAP      1   // MPR121 cap-touch       -> /dev/i2c-0 @ 0x5a
#define I2C_OLED     2   // SSD1306-class OLED      -> /dev/i2c-1 @ 0x3c
#define I2C_DEVICES  3

#define ACC_I2C_ADDR      0x19
#define CAP_I2C_ADDR      0x5a
#define OLED_I2C_ADDRESS  0x3c

#define I2C_BUS       0   // cap-touch + accelerometer
#define I2C_BUS_OLED  1   // OLED

// Open every device's bus and select its address. Returns 0 on success.
int i2c_init(void);
// Close all open device fds. Returns 0.
int i2c_deinit(void);

// Write [reg, val]. Returns 0 on success.
int i2cset(int device, uint8_t reg, uint8_t val);
// Write [reg, val1, val2]. Returns 0 on success.
int i2cset_w(int device, uint8_t reg, uint8_t val1, uint8_t val2);
// Write [reg] then read `bytes` into val. Returns 0 on success.
int i2cget(int device, uint8_t reg, uint8_t *val, int bytes);
// Write [reg, value x length]. Returns 0 on success.
int i2cfill(int device, uint8_t reg, uint8_t value, int length);
// Write [reg] then `length` bytes gathered from `data` with the given stride
// (`interleave`). Used to push transposed framebuffer columns to the OLED.
int i2cwrite_columns(int device, uint8_t reg, uint8_t *data, int length, int interleave);

#ifdef __cplusplus
}
#endif

#endif  // VARAN_HAL_I2C_H_
