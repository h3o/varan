// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Phonicbloom Ltd.
//
// cmd_listener.c — see cmd_listener.h.
//
// Two threads: one drains the FIFO (reopening it whenever the last writer
// closes), one accepts socket clients and serves them (one at a time — control
// traffic is light). Both share serve_fd(), which line-buffers and dispatches
// each complete line to the engine.

#include "audio/cmd_listener.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "audio/engine.h"
#include "protocol/command.h"

#define RUN_DIR   "/tmp/varan"
#define FIFO_PATH RUN_DIR "/cmd"
#define SOCK_PATH RUN_DIR "/ctl"

static pthread_t fifo_thr, sock_thr;
static int       running = 0;
static int       listen_fd = -1;

static void handle_line(const char *line, int reply_fd) {
  varan_cmd c;
  if (varan_cmd_parse(line, &c) != 0) return;  // blank
  char resp[VARAN_CMD_ARG_MAX + 256];
  engine_apply(&c, reply_fd >= 0 ? resp : NULL, sizeof(resp));
  if (reply_fd >= 0) {
    size_t l = strlen(resp);
    if (l < sizeof(resp) - 1) resp[l++] = '\n';
    (void)!write(reply_fd, resp, l);
  }
}

// Read from fd until EOF/error, dispatching each newline-terminated line.
// reply != 0 sends a response line back on the same fd.
static void serve_fd(int fd, int reply) {
  char buf[4096];
  size_t len = 0;
  for (;;) {
    ssize_t r = read(fd, buf + len, sizeof(buf) - 1 - len);
    if (r <= 0) {
      if (r < 0 && errno == EINTR) continue;
      break;  // EOF or error
    }
    len += (size_t)r;
    char *start = buf;
    char *nl;
    while ((nl = (char *)memchr(start, '\n', (size_t)(buf + len - start)))) {
      *nl = 0;
      handle_line(start, reply ? fd : -1);
      start = nl + 1;
    }
    size_t leftover = (size_t)(buf + len - start);
    memmove(buf, start, leftover);
    len = leftover;
    if (len >= sizeof(buf) - 1) len = 0;  // overlong line: drop it
  }
}

static void *fifo_loop(void *arg) {
  (void)arg;
  while (running) {
    // Blocks until a writer opens the FIFO; returns EOF when all writers close.
    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd < 0) {
      if (!running) break;
      usleep(200000);
      continue;
    }
    serve_fd(fd, 0);
    close(fd);
  }
  return 0;
}

static void *sock_loop(void *arg) {
  (void)arg;
  while (running) {
    int cfd = accept(listen_fd, NULL, NULL);
    if (cfd < 0) {
      if (!running) break;
      if (errno == EINTR) continue;
      usleep(100000);
      continue;
    }
    serve_fd(cfd, 1);
    close(cfd);
  }
  return 0;
}

int cmd_listener_start(void) {
  if (running) return 0;
  mkdir(RUN_DIR, 0755);  // ok if it already exists

  unlink(FIFO_PATH);
  if (mkfifo(FIFO_PATH, 0666) != 0 && errno != EEXIST) {
    fprintf(stderr, "cmd_listener: mkfifo %s: %s\n", FIFO_PATH, strerror(errno));
    return 1;
  }

  unlink(SOCK_PATH);
  listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    fprintf(stderr, "cmd_listener: socket: %s\n", strerror(errno));
    return 2;
  }
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);
  if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    fprintf(stderr, "cmd_listener: bind %s: %s\n", SOCK_PATH, strerror(errno));
    close(listen_fd);
    listen_fd = -1;
    return 3;
  }
  chmod(SOCK_PATH, 0666);
  if (listen(listen_fd, 4) != 0) {
    fprintf(stderr, "cmd_listener: listen: %s\n", strerror(errno));
    close(listen_fd);
    listen_fd = -1;
    return 4;
  }

  running = 1;
  pthread_create(&fifo_thr, NULL, fifo_loop, NULL);
  pthread_create(&sock_thr, NULL, sock_loop, NULL);
  printf("cmd_listener: FIFO %s + socket %s\n", FIFO_PATH, SOCK_PATH);
  return 0;
}

void cmd_listener_stop(void) {
  if (!running) return;
  running = 0;
  // Unblock the threads: cancel the blocking read()/accept() and unlink.
  pthread_cancel(fifo_thr);
  pthread_cancel(sock_thr);
  if (listen_fd >= 0) {
    close(listen_fd);
    listen_fd = -1;
  }
  pthread_join(fifo_thr, NULL);
  pthread_join(sock_thr, NULL);
  unlink(FIFO_PATH);
  unlink(SOCK_PATH);
}

const char *cmd_listener_dir(void) { return RUN_DIR; }
