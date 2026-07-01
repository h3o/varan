// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// command.c — see command.h.

#include "protocol/command.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char *kNames[] = {
    "none", "play", "pause", "resume", "stop",
    "seek", "gain", "status", "quit", "unknown"};

const char *varan_cmd_name(varan_cmd_type t) {
  if (t < CMD_NONE || t > CMD_UNKNOWN) return "?";
  return kNames[t];
}

int varan_cmd_parse(const char *line, varan_cmd *out) {
  if (!line || !out) return 1;
  memset(out, 0, sizeof(*out));
  out->type = CMD_NONE;

  const char *p = line;
  while (*p && isspace((unsigned char)*p)) p++;
  if (!*p) return 1;  // blank line

  // Read the verb (lower-cased).
  char verb[16];
  size_t vi = 0;
  while (*p && !isspace((unsigned char)*p) && vi < sizeof(verb) - 1)
    verb[vi++] = (char)tolower((unsigned char)*p++);
  verb[vi] = 0;

  while (*p && isspace((unsigned char)*p)) p++;
  const char *arg = p;  // remainder of the line

  if      (!strcmp(verb, "play"))   out->type = CMD_PLAY;
  else if (!strcmp(verb, "pause"))  out->type = CMD_PAUSE;
  else if (!strcmp(verb, "resume")) out->type = CMD_RESUME;
  else if (!strcmp(verb, "stop"))   out->type = CMD_STOP;
  else if (!strcmp(verb, "seek"))   out->type = CMD_SEEK;
  else if (!strcmp(verb, "gain"))   out->type = CMD_GAIN;
  else if (!strcmp(verb, "status")) out->type = CMD_STATUS;
  else if (!strcmp(verb, "quit") || !strcmp(verb, "exit")) out->type = CMD_QUIT;
  else { out->type = CMD_UNKNOWN; return 0; }

  if (out->type == CMD_PLAY) {
    // Whole remainder is the path; trim trailing whitespace/newline.
    size_t n = strlen(arg);
    while (n > 0 && isspace((unsigned char)arg[n - 1])) n--;
    if (n >= VARAN_CMD_ARG_MAX) n = VARAN_CMD_ARG_MAX - 1;
    memcpy(out->arg, arg, n);
    out->arg[n] = 0;
  } else if (out->type == CMD_SEEK) {
    if (*arg == '+' || *arg == '-') out->relative = 1;
    out->num = atof(arg);
  } else if (out->type == CMD_GAIN) {
    out->num = atof(arg);
  }
  return 0;
}
