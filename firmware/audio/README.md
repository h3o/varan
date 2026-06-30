# firmware/audio/ — the audio engine

The single in-process audio data-path:

```
file → decode → ring buffer → resample (speed) → FX → ALSA out
                    ▲                                      │
              producer thread                       one audio clock
```

- **One clock.** ALSA output paces everything; the decoder fills a **lock-free
  ring buffer** ahead of it. No process boundaries in this path.
- **Decoders** (permissive/PD only), dispatched by extension/magic:
  minimp3 (MP3), dr_flac/dr_wav (FLAC/WAV), stb_vorbis (OGG), libopus (Opus).
  FFmpeg-LGPL (no `--enable-gpl`) is the fallback if we want AAC/ALAC/streams.
- **Speed / "tuning"** — tape-style (speed+pitch coupled) via **libsamplerate**
  variable ratio. Pitch-preserving stretch (SoundTouch) is deferred.
- **Effects** — Freeverb (reverb), an in-house delay/flanger, and bs2b crossfeed
  for the headphone "3D" effect. All parameterizable over the command socket.
- **Metadata** — TagLib, read off the audio thread, surfaced to the OLED.

## Real-time rules (non-negotiable)

The audio callback/thread does **no `malloc`, no file I/O, no locks**. Buffers
are preallocated; file reads and tag parsing happen on producer/helper threads
and hand data over via the ring buffer. The target is zero xruns on a single
Cortex-A7 with ~55 MB usable RAM.

_Empty for now — first milestone is one decoder (MP3) → ring buffer → ALSA._
