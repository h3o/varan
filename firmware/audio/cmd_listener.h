// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// cmd_listener.h — command intake for the audio engine.
//
// Exposes the control protocol (protocol/command.h) over two transports under
// the run dir: a write-only FIFO ("cmd", fire-and-forget, ideal for scripts:
// `echo play /mnt/SD/x.mp3 > /tmp/varan/cmd`) and a Unix stream socket ("ctl",
// which also writes a one-line reply per command, e.g. for `status`).

#ifndef VARAN_AUDIO_CMD_LISTENER_H_
#define VARAN_AUDIO_CMD_LISTENER_H_

#ifdef __cplusplus
extern "C" {
#endif

// Create the FIFO + socket and start the listener threads. Returns 0 on success.
int  cmd_listener_start(void);
void cmd_listener_stop(void);
const char *cmd_listener_dir(void);

#ifdef __cplusplus
}
#endif

#endif  // VARAN_AUDIO_CMD_LISTENER_H_
