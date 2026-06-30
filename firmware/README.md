# firmware/ — open firmware components

The open building blocks of the Pocket Varan firmware. These are linked and
driven by the (closed) player application, but they are designed to stand on
their own and to be reusable by anyone building on the hardware.

## Subtrees

- **`hal/`** — hardware abstraction layer. One thin module per peripheral
  (OLED, MIDI, ALSA, GPIO buttons + rocker, LEDs, MPR121 cap-touch, LIS3DH
  accelerometer). Ported and trimmed from the prototype's working drivers.
- **`audio/`** — the audio engine: decoder dispatch, the lock-free ring buffer,
  the resampler (tape-style speed), and the effects chain. The whole data-path
  is one in-process pipeline.
- **`protocol/`** — the command-socket protocol: the text/binary command set
  (`play`, `seek`, `speed`, `loop`, FX params…) plus a reference client. This is
  the control seam, and the same surface plugins use.

## Boundaries (open-core)

This tree is open (MIT). The polished player **application** and the **board**
files are not here. The application talks to this code, and to plugins, only
across the command socket in `protocol/` — keeping the open/closed line and the
plugin API the same line.

## Constraints to respect

Single Cortex-A7, **64 MB RAM**. CPU is not the bottleneck; memory and real-time
discipline are. In the audio thread: **no `malloc`, no blocking I/O, no locks** —
preallocate, and move file/metadata work to other threads.
