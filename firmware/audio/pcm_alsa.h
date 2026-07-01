// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// pcm_alsa.h — minimal ALSA playback for the Pocket Varan.
//
// The V3s internal codec is card 0; its mixer (DAC/Headphone) is set up at boot
// by linux/init.d/S46sound, so this layer only opens a PCM and writes S16_LE
// interleaved frames. We open "plughw:0,0" with soft-resample so files at 44.1k
// or 48k play without us owning a resampler yet (that arrives with tape-speed).

#ifndef VARAN_AUDIO_PCM_ALSA_H_
#define VARAN_AUDIO_PCM_ALSA_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Open the codec for playback at rate/channels. Closes any previous handle.
// Returns 0 on success.
int  pcm_open(unsigned rate, unsigned channels);

// Write nframes interleaved frames (blocking). Recovers from underruns.
// Returns nframes on success, negative on fatal error.
int  pcm_write(const int16_t *frames, int nframes);

void pcm_drain(void);
void pcm_close(void);
int  pcm_is_open(void);

#ifdef __cplusplus
}
#endif

#endif  // VARAN_AUDIO_PCM_ALSA_H_
