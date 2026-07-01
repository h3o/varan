// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// resampler.h — variable-ratio resampler seam for tape-speed playback.
//
// Wraps libsamplerate (vendored, BSD-2) behind a small int16-in/int16-out API so
// the engine never touches the SRC types directly and the implementation stays
// swappable. "ratio" is out_rate/in_rate = 1/speed: <1 = faster+higher (tape
// sped up), >1 = slower+lower.

#ifndef VARAN_AUDIO_RESAMPLER_H_
#define VARAN_AUDIO_RESAMPLER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct resampler resampler;

// Create a resampler for `channels` (1 or 2). NULL on failure.
resampler *resampler_create(int channels);
void       resampler_destroy(resampler *r);
void       resampler_reset(resampler *r);  // call on seek / new file

// Resample interleaved int16 `in` (in_frames) to interleaved int16 `out`
// (capacity out_cap frames) at `ratio`. Sets *in_used / *out_gen (frames).
// Returns 0 on success.
int resampler_process(resampler *r, double ratio,
                      const int16_t *in, int in_frames,
                      int16_t *out, int out_cap,
                      int *in_used, int *out_gen);

#ifdef __cplusplus
}
#endif

#endif  // VARAN_AUDIO_RESAMPLER_H_
