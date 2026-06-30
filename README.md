# Pocket Varan — open core

*because MP3 players ain't rocket science*

**Pocket Varan** is a pocket-sized handheld smart audio player: gapless
multi-format playback (MP3, FLAC, OGG, Opus, WAV…) with **MIDI-CC control of
playback** — tape-style speed / "tuning", seeking, play-head jumps, loop slices
— plus an effects section (reverb, delay, headphone crossfeed). It runs Linux on
an Allwinner V3s.

This repository is the **open core**: everything you need to understand the
hardware, build the system image, and write firmware or plugins for the device.
It is MIT-licensed.

## Layout

| Path | Contents |
|---|---|
| `linux/` | Buildroot / kernel / u-boot / busybox configs, device tree, init scripts |
| `firmware/hal/` | Hardware abstraction: OLED, MIDI, ALSA, buttons, LEDs, MPR121, LIS3DH |
| `firmware/audio/` | Open audio engine: decoder glue, ring buffer, resampler, effects |
| `firmware/protocol/` | The **command-socket** protocol — the control seam & plugin API |
| `sdk/` | Plugin SDK headers + an example plugin |
| `docs/` | Architecture, command protocol, and hardware map |

## Architecture in one paragraph

The audio data-path is a single in-process engine — `decode → resample → FX →
ALSA out` — with one clock and a lock-free ring buffer (single Cortex-A7, 64 MB
RAM; no process boundaries in the audio path). System tasks (boot, SD mount, USB
gadget, MIDI prescaler, power, updates) are shell scripts in `linux/`. Everything
controllable — transport, speed, loops, effect params — is driven over a
**command socket**, which is simultaneously the scripting seam and the plugin
API. See [`docs/architecture.md`](docs/architecture.md).

## Hardware at a glance

Allwinner V3s (Cortex-A7 ~1.2 GHz) · **64 MB RAM** (the hard constraint) · 32 MB
SPI-NOR + microSD · internal codec (HP out + electret mic) · MIDI on UART1 via
3.5 mm jacks · USB-C gadget (serial + mass-storage) · GPIO buttons + rocker
(no knobs) · MPR121 cap-touch · SSD1306-class OLED · ~27 LEDs · 1500–2000 mAh
LiPo. Full map in [`docs/hardware.md`](docs/hardware.md).

## Status

Early. The Linux BSP boots and all peripherals are verified. The open firmware
engine and the command protocol are being brought up — see the docs for the
intended shape, and expect things to move.

## License

MIT — see [`LICENSE`](LICENSE). The shipping player application and the PCB
design files are not part of this repository.
