// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// hal.h — Pocket Varan hardware abstraction contract.
//
// This is the board-facing seam: the open audio engine and the navigate-only
// app speak to peripherals only through these functions, never through
// board-specific symbols. The implementations are migrated, per board, from the
// prototype's hw/ drivers (see firmware/MIGRATION.md for the mapping).

#ifndef VARAN_HAL_H_
#define VARAN_HAL_H_

#ifdef __cplusplus
extern "C" {
#endif

// --- LEDs (the ~27 chip LEDs) ---------------------------------------------
// Maps to prototype: init_leds_gingko / startup_leds_animation_gingko.
void hal_leds_init(void);
void hal_leds_startup_animation(void);
void hal_leds_all_off(void);

// --- Capacitive keyboard (MPR121 on i2c0 @0x5a, 12 electrodes) -------------
// Maps to: init_keyboard_ginkgo_MPR121 / keys_driver_VARAN[ _stop ].
// hal_keys_init() returns 0 on success. hal_keys_start() launches the
// background key-scan thread; hal_keys_stop() tears it down.
int  hal_keys_init(void);
void hal_keys_start(void);
void hal_keys_stop(void);

// --- MIDI (UART1 -> /dev/ttyS0 @ 31250 baud) ------------------------------
// Maps to: MIDI_driver_init. The 31250 prescaler is applied by
// linux/root/uart_prescaler.sh before the app starts.
int  hal_midi_init(void);

// --- I2C bus lifecycle ----------------------------------------------------
// Maps to: i2c_init / i2c_deinit.
int  hal_i2c_init(void);
void hal_i2c_deinit(void);

#ifdef __cplusplus
}
#endif

#endif  // VARAN_HAL_H_
