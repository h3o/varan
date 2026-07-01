// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// command.h — the Pocket Varan control protocol.
//
// This is the open, documented seam between the (closed) app / any external
// client and the (open) audio engine — the same text protocol is spoken over
// the command FIFO and the Unix control socket (see audio/cmd_listener.c) and
// documented in docs/command-protocol.md. Keep it dependency-free so it can be
// reused as an SDK piece.

#ifndef VARAN_PROTOCOL_COMMAND_H_
#define VARAN_PROTOCOL_COMMAND_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  CMD_NONE = 0,
  CMD_PLAY,    // play <path>
  CMD_PAUSE,   // pause
  CMD_RESUME,  // resume
  CMD_STOP,    // stop
  CMD_SEEK,    // seek <sec> | seek +<sec> | seek -<sec>
  CMD_GAIN,    // gain <0..1>
  CMD_STATUS,  // status
  CMD_QUIT,    // quit
  CMD_UNKNOWN
} varan_cmd_type;

#define VARAN_CMD_ARG_MAX 1024

typedef struct {
  varan_cmd_type type;
  char           arg[VARAN_CMD_ARG_MAX];  // path for CMD_PLAY
  double         num;                     // numeric arg (seek/gain)
  int            relative;                // CMD_SEEK: 1 if +/- prefix given
} varan_cmd;

// Parse one command line (trailing newline optional). Returns 0 if a verb was
// recognised (type may be CMD_UNKNOWN for a non-empty but unknown verb); returns
// non-zero for null/blank input.
int         varan_cmd_parse(const char *line, varan_cmd *out);
const char *varan_cmd_name(varan_cmd_type t);

#ifdef __cplusplus
}
#endif

#endif  // VARAN_PROTOCOL_COMMAND_H_
