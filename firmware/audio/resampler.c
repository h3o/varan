// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// resampler.c — see resampler.h. libsamplerate (SRC_SINC_FASTEST) wrapper.

#include "audio/resampler.h"

#include <stdlib.h>

#include "samplerate.h"  // vendored libsamplerate (extern "C")

// Scratch sizing. The engine feeds <= 2048 input frames per call; at the slowest
// speed (0.25 => ratio 4) that yields ~8192 output frames.
#define RS_IN_MAX   2048
#define RS_OUT_MAX  8192

struct resampler {
  SRC_STATE *state;
  int        channels;
  float     *in_f;   // RS_IN_MAX  * channels
  float     *out_f;  // RS_OUT_MAX * channels
};

resampler *resampler_create(int channels) {
  if (channels < 1) channels = 1;
  resampler *r = (resampler *)calloc(1, sizeof(*r));
  if (!r) return 0;
  r->channels = channels;
  int err = 0;
  r->state = src_new(SRC_SINC_FASTEST, channels, &err);
  r->in_f  = (float *)malloc((size_t)RS_IN_MAX * channels * sizeof(float));
  r->out_f = (float *)malloc((size_t)RS_OUT_MAX * channels * sizeof(float));
  if (!r->state || !r->in_f || !r->out_f) {
    resampler_destroy(r);
    return 0;
  }
  return r;
}

void resampler_destroy(resampler *r) {
  if (!r) return;
  if (r->state) src_delete(r->state);
  free(r->in_f);
  free(r->out_f);
  free(r);
}

void resampler_reset(resampler *r) {
  if (r && r->state) src_reset(r->state);
}

int resampler_process(resampler *r, double ratio,
                      const int16_t *in, int in_frames,
                      int16_t *out, int out_cap,
                      int *in_used, int *out_gen) {
  if (in_used) *in_used = 0;
  if (out_gen) *out_gen = 0;
  if (!r || !r->state) return 1;

  if (in_frames > RS_IN_MAX) in_frames = RS_IN_MAX;
  if (out_cap  > RS_OUT_MAX) out_cap  = RS_OUT_MAX;
  int ch = r->channels;

  src_short_to_float_array(in, r->in_f, in_frames * ch);

  SRC_DATA d;
  d.data_in       = r->in_f;
  d.input_frames  = in_frames;
  d.data_out      = r->out_f;
  d.output_frames = out_cap;
  d.src_ratio     = ratio;
  d.end_of_input  = 0;
  if (src_process(r->state, &d) != 0) return 2;

  src_float_to_short_array(r->out_f, out, (int)(d.output_frames_gen * ch));
  if (in_used) *in_used = (int)d.input_frames_used;
  if (out_gen) *out_gen = (int)d.output_frames_gen;
  return 0;
}
