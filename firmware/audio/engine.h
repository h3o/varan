// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// engine.h — the Pocket Varan audio engine (Phase 1, Slice 1).
//
// A single background thread owns the decoder and drives decode -> ALSA. All
// control is issued as intent flags under a lock, so the FIFO/socket listeners
// and the UI can steer playback without touching the decoder. Slice 1 handles
// MP3 (minimp3) at native rate; resampling / tape-speed and more formats come
// next.

#ifndef VARAN_AUDIO_ENGINE_H_
#define VARAN_AUDIO_ENGINE_H_

#include <stddef.h>

#include "protocol/command.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { ENG_STOPPED = 0, ENG_PLAYING, ENG_PAUSED } engine_state;

int  engine_init(void);      // start the audio thread
void engine_shutdown(void);  // stop the thread, close the PCM

// Control — thread-safe; callable from the listeners and the UI.
int  engine_play(const char *path);
void engine_pause(void);
void engine_resume(void);
void engine_stop(void);
void engine_seek(double seconds, int relative);
void engine_set_gain(double gain01);

// Dispatch a parsed command; if resp is non-NULL, writes a one-line reply.
void engine_apply(const varan_cmd *cmd, char *resp, size_t resp_n);

// "state=.. file=.. pos=.. dur=.. gain=.." snapshot.
void engine_status(char *buf, size_t n);

// Set by the QUIT command; the app main loop should exit when true.
int  engine_quit_requested(void);

#ifdef __cplusplus
}
#endif

#endif  // VARAN_AUDIO_ENGINE_H_
