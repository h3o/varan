// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// engine.c — see engine.h.
//
// Threading model: exactly one thread (engine_thread) ever touches the decoder
// and the PCM. Control functions only set intent under `mtx` and signal `cv`;
// the thread consumes those at the top of each loop. Shared status (state, pos,
// dur, file, gain) is read/written under `mtx`, so listeners/UI never race the
// decoder.

#include "audio/engine.h"

#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "audio/pcm_alsa.h"
#include "audio/resampler.h"
#include "minimp3_ex.h"

#define READ_FRAMES 2048  // frames decoded per chunk (~46ms @44.1k)
#define OUT_FRAMES  8192  // resampled output headroom (2048 in * ratio 4)

// --- synchronisation -------------------------------------------------------
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cv  = PTHREAD_COND_INITIALIZER;
static pthread_t       thr;
static int             thread_running = 0;
static int             thr_quit = 0;   // tell the thread to exit
static int             app_quit = 0;   // QUIT command -> app should exit

// --- shared state (guarded by mtx) -----------------------------------------
static engine_state state = ENG_STOPPED;
static float        gain  = 1.0f;
static float        speed = 1.0f;  // playback speed (tape-style)
static char         cur_file[VARAN_CMD_ARG_MAX];
static double       cur_pos = 0.0;  // seconds
static double       cur_dur = 0.0;  // seconds

// pending requests (guarded by mtx)
static int    req_play = 0, req_stop = 0, req_pause = 0, req_resume = 0, req_seek = 0;
static char   req_path[VARAN_CMD_ARG_MAX];
static double req_seek_sec = 0.0;
static int    req_seek_rel = 0;

// --- decoder + resampler (thread-private) ----------------------------------
static mp3dec_ex_t dec;
static int         dec_open = 0;
static int         dec_channels = 2;
static int         dec_hz = 44100;
static resampler  *rs = 0;

static void apply_gain_i16(int16_t *b, int n, float g) {
  if (g >= 0.999f) return;
  for (int i = 0; i < n; i++) b[i] = (int16_t)(b[i] * g);
}

static void decoder_close(void) {
  if (dec_open) {
    mp3dec_ex_close(&dec);
    dec_open = 0;
  }
}

static int decoder_open(const char *path) {
  decoder_close();
  memset(&dec, 0, sizeof(dec));
  if (mp3dec_ex_open(&dec, path, MP3D_SEEK_TO_SAMPLE) != 0 || dec.info.hz == 0) {
    fprintf(stderr, "engine: cannot open MP3 '%s'\n", path);
    return 1;
  }
  dec_channels = dec.info.channels ? dec.info.channels : 2;
  dec_hz = dec.info.hz;
  dec_open = 1;
  return 0;
}

// --- helpers (call with mtx held) ------------------------------------------
static void set_stopped_locked(void) {
  state = ENG_STOPPED;
  cur_pos = 0.0;
  cur_dur = 0.0;
  cur_file[0] = 0;
}

static void *engine_thread(void *arg) {
  (void)arg;
  static int16_t in_buf[READ_FRAMES * 2];   // decoded (MP3 is at most 2ch)
  static int16_t out_buf[OUT_FRAMES * 2];   // resampled

  for (;;) {
    // ---- wait for something to do, then snapshot requests ----
    pthread_mutex_lock(&mtx);
    while (!thr_quit && state != ENG_PLAYING && !req_play && !req_stop &&
           !req_pause && !req_resume && !req_seek) {
      pthread_cond_wait(&cv, &mtx);
    }
    if (thr_quit) {
      pthread_mutex_unlock(&mtx);
      break;
    }
    int do_play = req_play, do_stop = req_stop, do_pause = req_pause,
        do_resume = req_resume, do_seek = req_seek;
    char path[VARAN_CMD_ARG_MAX];
    memcpy(path, req_path, sizeof(path));
    double seek_sec = req_seek_sec;
    int seek_rel = req_seek_rel;
    float g = gain;
    float spd = speed;
    req_play = req_stop = req_pause = req_resume = req_seek = 0;
    pthread_mutex_unlock(&mtx);

    // ---- apply requests (decoder owned here, no lock needed for it) ----
    if (do_stop) {
      decoder_close();
      resampler_destroy(rs);
      rs = 0;
      pcm_close();
      pthread_mutex_lock(&mtx);
      set_stopped_locked();
      pthread_mutex_unlock(&mtx);
    }

    if (do_play) {
      decoder_close();
      resampler_destroy(rs);
      rs = 0;
      pcm_close();
      if (decoder_open(path) == 0 && pcm_open(dec_hz, dec_channels) == 0) {
        rs = resampler_create(dec_channels);  // NULL -> engine falls back to 1x
        pthread_mutex_lock(&mtx);
        state = ENG_PLAYING;
        strncpy(cur_file, path, sizeof(cur_file) - 1);
        cur_file[sizeof(cur_file) - 1] = 0;
        cur_dur = (double)dec.samples / dec_channels / dec_hz;
        cur_pos = 0.0;
        pthread_mutex_unlock(&mtx);
      } else {
        decoder_close();
        pcm_close();
        pthread_mutex_lock(&mtx);
        set_stopped_locked();
        pthread_mutex_unlock(&mtx);
      }
    }

    if (do_seek && dec_open) {
      double base = (seek_rel ? cur_pos : 0.0) + seek_sec;
      if (base < 0.0) base = 0.0;
      uint64_t target = (uint64_t)(base * dec_hz) * (uint64_t)dec_channels;
      if (target > dec.samples) target = dec.samples;
      mp3dec_ex_seek(&dec, target);
      resampler_reset(rs);  // drop filter history across the jump
      pthread_mutex_lock(&mtx);
      cur_pos = (double)dec.cur_sample / dec_channels / dec_hz;
      pthread_mutex_unlock(&mtx);
    }

    if (do_pause) {
      pthread_mutex_lock(&mtx);
      if (state == ENG_PLAYING) state = ENG_PAUSED;
      pthread_mutex_unlock(&mtx);
    }
    if (do_resume) {
      pthread_mutex_lock(&mtx);
      if (state == ENG_PAUSED) state = ENG_PLAYING;
      pthread_mutex_unlock(&mtx);
    }

    // ---- if playing, decode + write one chunk (paced by blocking write) ----
    pthread_mutex_lock(&mtx);
    engine_state st = state;
    pthread_mutex_unlock(&mtx);

    if (st == ENG_PLAYING && dec_open) {
      size_t want = (size_t)READ_FRAMES * dec_channels;
      size_t got = mp3dec_ex_read(&dec, in_buf, want);
      if (got == 0) {
        // end of file
        pcm_drain();
        decoder_close();
        resampler_destroy(rs);
        rs = 0;
        pcm_close();
        pthread_mutex_lock(&mtx);
        cur_pos = cur_dur;
        state = ENG_STOPPED;
        pthread_mutex_unlock(&mtx);
      } else {
        int in_frames = (int)(got / dec_channels);
        if (rs) {
          // Tape-speed: resample by ratio = 1/speed, feeding the whole chunk
          // (may take several calls if the output buffer fills first).
          double ratio = 1.0 / (spd > 0.0f ? spd : 1.0);
          int off = 0;
          while (off < in_frames) {
            int in_used = 0, out_gen = 0;
            if (resampler_process(rs, ratio, in_buf + off * dec_channels,
                                   in_frames - off, out_buf, OUT_FRAMES,
                                   &in_used, &out_gen) != 0)
              break;
            if (out_gen > 0) {
              apply_gain_i16(out_buf, out_gen * dec_channels, g);
              pcm_write(out_buf, out_gen);
            }
            if (in_used <= 0 && out_gen <= 0) break;  // safety
            off += in_used;
          }
        } else {
          apply_gain_i16(in_buf, (int)got, g);
          pcm_write(in_buf, in_frames);
        }
        pthread_mutex_lock(&mtx);
        cur_pos = (double)dec.cur_sample / dec_channels / dec_hz;
        pthread_mutex_unlock(&mtx);
      }
    }
  }

  decoder_close();
  resampler_destroy(rs);
  rs = 0;
  pcm_close();
  return 0;
}

// --- lifecycle -------------------------------------------------------------
int engine_init(void) {
  if (thread_running) return 0;
  thr_quit = 0;
  app_quit = 0;
  state = ENG_STOPPED;
  if (pthread_create(&thr, NULL, engine_thread, NULL) != 0) {
    fprintf(stderr, "engine_init: cannot create audio thread\n");
    return 1;
  }
  thread_running = 1;
  printf("engine_init: audio engine started\n");
  return 0;
}

void engine_shutdown(void) {
  if (!thread_running) return;
  pthread_mutex_lock(&mtx);
  thr_quit = 1;
  pthread_cond_signal(&cv);
  pthread_mutex_unlock(&mtx);
  pthread_join(thr, NULL);
  thread_running = 0;
}

// --- control ---------------------------------------------------------------
int engine_play(const char *path) {
  if (!path || !*path) return 1;
  pthread_mutex_lock(&mtx);
  strncpy(req_path, path, sizeof(req_path) - 1);
  req_path[sizeof(req_path) - 1] = 0;
  req_play = 1;
  pthread_cond_signal(&cv);
  pthread_mutex_unlock(&mtx);
  return 0;
}

void engine_pause(void) {
  pthread_mutex_lock(&mtx);
  req_pause = 1;
  pthread_cond_signal(&cv);
  pthread_mutex_unlock(&mtx);
}

void engine_resume(void) {
  pthread_mutex_lock(&mtx);
  req_resume = 1;
  pthread_cond_signal(&cv);
  pthread_mutex_unlock(&mtx);
}

void engine_stop(void) {
  pthread_mutex_lock(&mtx);
  req_stop = 1;
  pthread_cond_signal(&cv);
  pthread_mutex_unlock(&mtx);
}

void engine_seek(double seconds, int relative) {
  pthread_mutex_lock(&mtx);
  req_seek = 1;
  req_seek_sec = seconds;
  req_seek_rel = relative;
  pthread_cond_signal(&cv);
  pthread_mutex_unlock(&mtx);
}

void engine_set_speed(double ratio) {
  if (ratio < 0.25) ratio = 0.25;
  if (ratio > 4.0) ratio = 4.0;
  pthread_mutex_lock(&mtx);
  speed = (float)ratio;
  pthread_mutex_unlock(&mtx);
}

void engine_set_gain(double gain01) {
  if (gain01 < 0.0) gain01 = 0.0;
  if (gain01 > 1.0) gain01 = 1.0;
  pthread_mutex_lock(&mtx);
  gain = (float)gain01;
  pthread_mutex_unlock(&mtx);
}

void engine_status(char *buf, size_t n) {
  pthread_mutex_lock(&mtx);
  const char *s = state == ENG_PLAYING ? "playing"
                  : state == ENG_PAUSED ? "paused"
                                        : "stopped";
  snprintf(buf, n, "state=%s file=%s pos=%.2f dur=%.2f speed=%.3f gain=%.2f", s,
           cur_file[0] ? cur_file : "-", cur_pos, cur_dur, speed, gain);
  pthread_mutex_unlock(&mtx);
}

int engine_quit_requested(void) {
  int q;
  pthread_mutex_lock(&mtx);
  q = app_quit;
  pthread_mutex_unlock(&mtx);
  return q;
}

void engine_apply(const varan_cmd *c, char *resp, size_t rn) {
  char sbuf[VARAN_CMD_ARG_MAX + 128];
  switch (c->type) {
    case CMD_PLAY:
      engine_play(c->arg);
      if (resp) snprintf(resp, rn, "OK play %s", c->arg);
      break;
    case CMD_PAUSE:
      engine_pause();
      if (resp) snprintf(resp, rn, "OK pause");
      break;
    case CMD_RESUME:
      engine_resume();
      if (resp) snprintf(resp, rn, "OK resume");
      break;
    case CMD_STOP:
      engine_stop();
      if (resp) snprintf(resp, rn, "OK stop");
      break;
    case CMD_SEEK:
      engine_seek(c->num, c->relative);
      if (resp) snprintf(resp, rn, "OK seek %.2f%s", c->num,
                         c->relative ? " (rel)" : "");
      break;
    case CMD_SPEED:
      engine_set_speed(c->num);
      if (resp) snprintf(resp, rn, "OK speed %.3f", c->num);
      break;
    case CMD_GAIN:
      engine_set_gain(c->num);
      if (resp) snprintf(resp, rn, "OK gain %.2f", c->num);
      break;
    case CMD_STATUS:
      engine_status(sbuf, sizeof(sbuf));
      if (resp) snprintf(resp, rn, "%s", sbuf);
      break;
    case CMD_QUIT:
      pthread_mutex_lock(&mtx);
      app_quit = 1;
      pthread_mutex_unlock(&mtx);
      if (resp) snprintf(resp, rn, "OK quit");
      break;
    default:
      if (resp) snprintf(resp, rn, "ERR unknown command");
      break;
  }
}
