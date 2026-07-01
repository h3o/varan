// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// leds.h — LED driver for the Pocket Varan (V3s / VR29x board).
//
// The board has 24 LEDs: six status LEDs plus a ring of nine clock positions,
// each with an outer (blue) and inner (green) LED. Pin directions are set once
// at boot by linux/root/all_LEDs_init.sh (sysfs); this driver only flips the
// output DATA bits, which it does by mmap()ing the V3s PIO block directly. That
// keeps per-LED updates near-free and lets the whole port-E ring change in a
// single register write (the basis for future PWM / smooth animation).
//
// Polarity: the red power LED (PE12) is active-high; every other LED is
// active-low. leds_set(id, 1) lights a LED regardless — the map handles it.

#ifndef VARAN_HAL_LEDS_H_
#define VARAN_HAL_LEDS_H_

#ifdef __cplusplus
extern "C" {
#endif

// Logical LED ids. Ring entries are ordered clockwise starting at 12 o'clock
// (the 3/6/9 o'clock positions are where the buttons sit, so they're absent).
enum {
  // status LEDs
  LED_POWER = 0,  // PE12 red    (active-high)
  LED_USB,        // PF6  yellow
  LED_START,      // PB4  green
  LED_STOP,       // PG0  orange
  LED_SIG,        // PE22 blue
  LED_CENTER,     // PB5  orange (central)
  // ring — outer (blue), clockwise from 12 o'clock
  LED_R12_OUT, LED_R1_OUT, LED_R2_OUT, LED_R4_OUT, LED_R5_OUT,
  LED_R7_OUT, LED_R8_OUT, LED_R10_OUT, LED_R11_OUT,
  // ring — inner (green), clockwise from 12 o'clock
  LED_R12_IN, LED_R1_IN, LED_R2_IN, LED_R4_IN, LED_R5_IN,
  LED_R7_IN, LED_R8_IN, LED_R10_IN, LED_R11_IN,
  LEDS_COUNT
};

// mmap the PIO block. Returns 0 on success, non-zero if /dev/mem can't be
// mapped (needs root) — callers should treat that as non-fatal.
int  leds_init(void);
void leds_deinit(void);

// on != 0 lights the LED (polarity handled internally).
void leds_set(int led, int on);
void leds_all(int on);
void leds_all_off(void);

// A short clockwise ring sweep + status flash, then settle with power on.
void leds_startup_animation(void);

#ifdef __cplusplus
}
#endif

#endif  // VARAN_HAL_LEDS_H_
