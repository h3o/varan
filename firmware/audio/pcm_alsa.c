// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// pcm_alsa.c — see pcm_alsa.h.

#include "audio/pcm_alsa.h"

#include <alsa/asoundlib.h>
#include <stdio.h>

// Card 0 = V3s internal codec. plughw adds format/rate/channel conversion.
#define PCM_DEVICE     "plughw:0,0"
#define PCM_LATENCY_US 500000  // ~0.5s of buffering — safe on a slow A7

static snd_pcm_t *pcm = 0;
static unsigned   pcm_channels = 2;

int pcm_open(unsigned rate, unsigned channels) {
  if (pcm) pcm_close();
  if (channels < 1) channels = 2;

  int err = snd_pcm_open(&pcm, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0) {
    fprintf(stderr, "pcm_open: %s: %s\n", PCM_DEVICE, snd_strerror(err));
    pcm = 0;
    return 1;
  }

  // soft_resample = 1 lets ALSA convert the source rate to the codec's rate.
  err = snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE,
                           SND_PCM_ACCESS_RW_INTERLEAVED, channels, rate,
                           1 /*soft_resample*/, PCM_LATENCY_US);
  if (err < 0) {
    fprintf(stderr, "pcm_open: set_params(%u Hz, %u ch): %s\n", rate, channels,
            snd_strerror(err));
    snd_pcm_close(pcm);
    pcm = 0;
    return 2;
  }

  pcm_channels = channels;
  printf("pcm_open: %s @ %u Hz, %u ch\n", PCM_DEVICE, rate, channels);
  return 0;
}

int pcm_write(const int16_t *frames, int nframes) {
  if (!pcm) return -1;
  const int16_t *p = frames;
  int remaining = nframes;
  while (remaining > 0) {
    snd_pcm_sframes_t n = snd_pcm_writei(pcm, p, remaining);
    if (n < 0) {
      // -EPIPE (underrun) / -ESTRPIPE (suspend): try to recover and continue.
      n = snd_pcm_recover(pcm, (int)n, 1 /*silent*/);
      if (n < 0) {
        fprintf(stderr, "pcm_write: %s\n", snd_strerror((int)n));
        return -2;
      }
      continue;
    }
    p += (size_t)n * pcm_channels;
    remaining -= (int)n;
  }
  return nframes;
}

void pcm_drain(void) {
  if (pcm) snd_pcm_drain(pcm);
}

void pcm_close(void) {
  if (pcm) {
    snd_pcm_close(pcm);
    pcm = 0;
  }
}

int pcm_is_open(void) {
  return pcm != 0;
}
