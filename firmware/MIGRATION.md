# Phase 0 migration manifest

How the navigate-only skeleton is carved out of the prototype synth framework
(`firmware/don-iguano-v165` in the working tree). The principle: **keep the glue
and the HAL, drop everything that generates or sequences sound.**

## What was already true

`Varan_UI::VARAN()` in the prototype is *already* a navigate-only loop — OLED +
file browser + GPIO buttons + MPR121 cap-touch keys, with **no synth engine
instantiated**. The only synth coupling was the class hierarchy:

```
Varan_UI  ->  Patches  ->  Engines (dsp/Engines.h)  ->  ~40 DSP engines
```

Phase 0 severs that: the new `app/varan_ui.cpp` inherits nothing and talks to
peripherals through `hal/hal.h`. That single cut removes the entire synth.

## Status: builds ✅

`./firmware/build.sh` cross-compiles the navigate-only firmware with the Linaro
GCC 5.5 toolchain into a real ARM hard-float ELF (`firmware/build/varan`). It
links only the standard libs — **no `libasound`, no synth** — confirming the cut
is clean. Migrated so far: the OLED stack (ssd1306 + Waveshare GUI/Fonts, MIT), a
clean `hal/i2c.{h,c}`, `hal/display.{h,cpp}`, and `app/browser.h`. Still stubbed
in `hal/hal.c` (next batch, from prototype `hw/`): LEDs, MPR121 cap-touch keys,
MIDI.

## Landed in this repo (clean, MIT, authored fresh)

| File | Role |
|---|---|
| `firmware/app/main.cpp` | linear bring-up entry (no board #ifdefs, no engine select) |
| `firmware/app/varan_ui.{h,cpp}` | navigate-only UI, decoupled from `Engines` |
| `firmware/hal/hal.h` + `hal.c` | the HAL contract + impl (i2c real; leds/keys/midi stubbed) |
| `firmware/hal/i2c.{h,c}` | minimal MIT i2c layer for VR294 (replaces the board-#ifdef `peripherals.c`) |
| `firmware/build.sh` | Linaro cross-compile |

## KEEP — migrate from prototype `hw/` (relicense → MIT, see below)

| Prototype source | Destination | Notes |
|---|---|---|
| `hw/Display.{cpp,h}` + `hw/oled/ssd1306.*` | `firmware/hal/` | OLED driver used by the browser |
| `hw/SDcard_browser.h` | `firmware/app/browser.h` | `FileSystemBrowser` |
| `hw/SDcard_utils.{cpp,h}` | `firmware/app/` | path/listing helpers |
| `hw/keyboard.{c,h}` | `firmware/hal/` | `init_keyboard_ginkgo_MPR121`, `keys_driver_VARAN[_stop]` |
| `hw/peripherals.{c,h}` (+ V3s parts only) | `firmware/hal/` | I2C/GPIO base; strip other-board variants |
| `hw/leds.{c,h}` | `firmware/hal/` | `init_leds_gingko`, `startup_leds_animation_gingko` |
| `hw/midi.{c,h}` | `firmware/hal/` | `MIDI_driver_init` |
| `hw/commands.{c,h}` | `firmware/hal/` | `i2c_init` / `i2c_deinit` |

`hw/Settings.*` and `hw/pots.*` are **deferred** — the navigate-only skeleton
doesn't use them (Settings came in via TrackEngine; this board has no pots).

## GUT — do NOT migrate

- **All of `dsp/`** — every engine/effect/filter (Antarctica, DCO_Synth,
  Granular, KS_Channel, Engines, Effects, Filters, …) and the `Patches`/`Engines`
  base classes. *(Exceptions reserved for later: `dsp/freeverb` (PD),
  `dsp/EffectDelay`, `dsp/EffectFlanger` (own) return in Phase 3.)*
- **`mi/`** (clouds, stmlib), **`ml_synth/`**, **`dkr/`** — synth engines.
- **`Siluria_UI.*`, `UI_Dispatcher.*`, `dsp/TrackEngine.*`** — other-product UI.
- **`MusicBox.*`, `EuclideanRhythms.*`, `Generators.*`, `Samples.*`** — sequencing.
- **Other-board / other-chip drivers** — `AMG88xx`, `RDA5807`,
  `SparkFunSi4703`, `VL53L5Cx`, `tft/`, `peripherals_{CN301,LS,MT186,RDA8810}`.
- **`libs/`** — `sox`, `nutekt`, `VL53L5CX_ULD_API` dropped; `cJSON` optional
  (only if the settings format needs JSON later).

## Licensing actions (gating the HAL copy)

1. **Relicense own HAL/glue files GPLv3 → MIT.** The prototype `hw/*` headers
   carry a GPLv3 notice but are Phonicbloom-owned; the open tree is MIT
   (`LICENSE`). Migrating them means updating each header to the MIT notice.
   **Needs an explicit go-ahead** before headers are rewritten — see
   [[varan-licensing-landmines]].
2. **Exclude CC-BY-NC-SA files.** `Interface.{h,cpp}`, `MusicBox.*`, `notes.*`
   are NonCommercial and say "must not be distributed separately" — they stay
   out of the open tree entirely. The skeleton already avoids them.
3. **Drop third-party GPL.** `dsp/mverb` (GPLv3) is not migrated.

## Build

`./firmware/build.sh` (Linaro GCC 5.5, `arm-linux-gnueabihf`, `-std=gnu++11`).
Everything is compiled as C++ — same as the prototype's autocompiler — so the
`.c` drivers link against the C++ TUs. Override the toolchain with
`TOOLCHAIN=/path/to/arm-linux-gnueabihf- ./build.sh` (note the trailing dash).
Output: `firmware/build/varan` (git-ignored).
