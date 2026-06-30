# Architecture

Pocket Varan is a media player, not a synth. The firmware is organized **by
plane**, not as one undifferentiated binary.

## Three planes

### 1. Audio data-plane — one in-process engine
`decode → resample → FX → ALSA out`, with a single clock and a lock-free ring
buffer. There are **no process boundaries in the audio path**: the SoC is a
single Cortex-A7 with 64 MB RAM, so splitting the hot path across processes would
cost IPC and memory for nothing. See [`../firmware/audio/`](../firmware/audio/).

### 2. System / control-plane — shell scripts
Boot, SD mount, USB gadget, the MIDI UART prescaler, power/shutdown, and firmware
update are shell scripts. They already exist and work — see
[`../linux/`](../linux/) (`init.d/` and `root/`). Keeping system glue as scripts
keeps it inspectable and hackable on-device.

### 3. UI + control surface — over a command socket
The UI runs in-process initially, but it talks to the engine through a **command
socket** (Unix domain socket / FIFO). The MIDI-CC mapper, scripts, and plugins
use that same socket. See [`../firmware/protocol/`](../firmware/protocol/).

## Why the command socket matters

It is three things at once:

1. **The scripting/modularity seam** — anything can drive playback.
2. **The open/closed boundary** — the closed player app and the open engine meet
   here; nothing reaches across it except commands.
3. **The plugin API** — third-party extensions are just other clients of the
   socket, so they are as capable as the first-party UI.

Drawing one line for all three is the central design decision.

## Constraints that shape everything

- **RAM (64 MB, ~55 MB usable) is the hard limit**, not CPU. MP3 ~5–10%, FLAC
  ~3–5%, resample a few %, reverb+delay+crossfeed < ~50% of one core combined.
- **Real-time discipline in the audio thread:** no `malloc`, no blocking I/O, no
  locks. Preallocate; do file and metadata work elsewhere and pass it through the
  ring buffer.
- **Reuse the prototype's glue, gut its synth engines.** The HAL, OLED/`Display`,
  SD browser, settings, and UI shell are kept; the sound-generation engines are
  removed.

## Library stack

Permissive/LGPL only, so the (closed) application stays unencumbered:
minimp3 (CC0), dr_flac/dr_wav (PD), stb_vorbis (PD), libopus (BSD),
libsamplerate (BSD-2), TagLib (LGPL), bs2b (MIT), Freeverb (PD). FFmpeg is an
option only when built **without** `--enable-gpl`. GPL components (e.g. MVerb,
Rubber Band, zita-resampler) are deliberately excluded.
