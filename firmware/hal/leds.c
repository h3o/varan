// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// leds.c — see leds.h. Direct-register LED driver for the Allwinner V3s PIO.
//
// The V3s pin controller lives at physical 0x01C20800; we mmap the enclosing
// 4K page (0x01C20000) of /dev/mem and address the per-port DATA registers by
// their standard sunxi offsets (GPIO_OFFSET 0x800). Only DATA is touched — the
// boot script (all_LEDs_init.sh) has already set these pins to outputs — so
// there's no risk of clobbering pinmux, and updates are single memory writes.

#include "hal/leds.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define PIO_PHYS_PAGE 0x01C20000u  // page containing the PIO block (@ +0x800)
#define PIO_MAP_LEN   0x1000u      // one 4K page covers all ports
#define WORD(byte_off) ((byte_off) / 4u)

// Per-port DATA register indexes (word offsets within the mapped page).
#define PB_DAT WORD(0x800 + 0x34)
#define PE_DAT WORD(0x800 + 0xA0)
#define PF_DAT WORD(0x800 + 0xC4)
#define PG_DAT WORD(0x800 + 0xE8)

enum { P_B, P_E, P_F, P_G };

static volatile uint32_t *gpio = 0;

typedef struct { uint8_t port; uint8_t bit; uint8_t active_high; } led_map_t;

// Order matches the enum in leds.h exactly (positional init).
static const led_map_t kLeds[LEDS_COUNT] = {
  // status
  {P_E, 12, 1},  // LED_POWER  (red, active-high)
  {P_F, 6,  0},  // LED_USB    (yellow)
  {P_B, 4,  0},  // LED_START  (green)
  {P_G, 0,  0},  // LED_STOP   (orange)
  {P_E, 22, 0},  // LED_SIG    (blue)
  {P_B, 5,  0},  // LED_CENTER (orange)
  // ring outer (blue), clockwise from 12
  {P_E, 16, 0},  // R12_OUT
  {P_G, 4,  0},  // R1_OUT
  {P_G, 3,  0},  // R2_OUT
  {P_E, 9,  0},  // R4_OUT
  {P_E, 7,  0},  // R5_OUT
  {P_E, 3,  0},  // R7_OUT
  {P_B, 0,  0},  // R8_OUT
  {P_B, 1,  0},  // R10_OUT
  {P_E, 0,  0},  // R11_OUT
  // ring inner (green), clockwise from 12
  {P_E, 17, 0},  // R12_IN
  {P_E, 15, 0},  // R1_IN
  {P_E, 10, 0},  // R2_IN
  {P_E, 8,  0},  // R4_IN
  {P_E, 6,  0},  // R5_IN
  {P_E, 4,  0},  // R7_IN
  {P_E, 2,  0},  // R8_IN
  {P_E, 1,  0},  // R10_IN
  {P_E, 5,  0},  // R11_IN
};

static volatile uint32_t *dat_reg(uint8_t port) {
  switch (port) {
    case P_B: return &gpio[PB_DAT];
    case P_E: return &gpio[PE_DAT];
    case P_F: return &gpio[PF_DAT];
    case P_G: return &gpio[PG_DAT];
  }
  return 0;
}

int leds_init(void) {
  if (gpio) return 0;
  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    fprintf(stderr, "leds_init: cannot open /dev/mem (need root)\n");
    return 1;
  }
  void *map = mmap(NULL, PIO_MAP_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                   PIO_PHYS_PAGE);
  close(fd);
  if (map == MAP_FAILED) {
    fprintf(stderr, "leds_init: mmap failed\n");
    return 2;
  }
  gpio = (volatile uint32_t *)map;
  printf("leds_init: PIO mapped, %d LEDs ready\n", LEDS_COUNT);
  return 0;
}

void leds_deinit(void) {
  if (gpio) {
    munmap((void *)gpio, PIO_MAP_LEN);
    gpio = 0;
  }
}

void leds_set(int led, int on) {
  if (!gpio || led < 0 || led >= LEDS_COUNT) return;
  const led_map_t *m = &kLeds[led];
  volatile uint32_t *reg = dat_reg(m->port);
  if (!reg) return;
  uint32_t mask = 1u << m->bit;
  // Output level to drive: active-high LED lights on 1, active-low on 0.
  int level = on ? m->active_high : !m->active_high;
  if (level) *reg |= mask;
  else       *reg &= ~mask;
}

void leds_all(int on) {
  for (int i = 0; i < LEDS_COUNT; i++) leds_set(i, on);
}

void leds_all_off(void) {
  leds_all(0);
}

void leds_startup_animation(void) {
  if (!gpio) return;

  // Ring ids in clockwise order (see enum): outer then inner run in parallel.
  static const int ring_out[9] = {
      LED_R12_OUT, LED_R1_OUT, LED_R2_OUT, LED_R4_OUT, LED_R5_OUT,
      LED_R7_OUT, LED_R8_OUT, LED_R10_OUT, LED_R11_OUT};
  static const int ring_in[9] = {
      LED_R12_IN, LED_R1_IN, LED_R2_IN, LED_R4_IN, LED_R5_IN,
      LED_R7_IN, LED_R8_IN, LED_R10_IN, LED_R11_IN};

  leds_all_off();

  // Clockwise "comet" around the ring.
  for (int i = 0; i < 9; i++) {
    leds_set(ring_out[i], 1);
    leds_set(ring_in[i], 1);
    usleep(45000);
    leds_set(ring_out[i], 0);
    leds_set(ring_in[i], 0);
  }

  // Two quick all-on flashes.
  for (int f = 0; f < 2; f++) {
    leds_all(1);
    usleep(80000);
    leds_all_off();
    usleep(80000);
  }

  // Settle: everything off, power LED lit.
  leds_all_off();
  leds_set(LED_POWER, 1);
}
