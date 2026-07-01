# Command protocol

The audio engine exposes a local, line-oriented **text** control protocol — the
open seam between the app / MIDI-CC mapper / shell scripts / plugins and the
engine. Whatever the UI can do, any client can do.

## Transports

Both live under the run dir (`/tmp/varan/`, see `cmd_listener_dir()`), created at
startup:

| Path | Kind | Direction | Use |
|---|---|---|---|
| `/tmp/varan/cmd` | FIFO | write-only, fire-and-forget | scripts / init: `echo play /mnt/SD/x.mp3 > /tmp/varan/cmd` |
| `/tmp/varan/ctl` | Unix stream socket | request → one-line reply | status queries, tooling: `socat - UNIX-CONNECT:/tmp/varan/ctl` |

Commands are newline-terminated. On the socket, each command gets exactly one
reply line (`OK ...`, a `status` line, or `ERR ...`). The FIFO discards replies.

## Implemented (Phase 1, Slice 1)

| Command | Meaning | Socket reply |
|---|---|---|
| `play PATH` | load and play an MP3 (whole remainder is the path) | `OK play PATH` |
| `pause` | pause, keep position | `OK pause` |
| `resume` | resume from pause | `OK resume` |
| `stop` | stop, release the file | `OK stop` |
| `seek SEC` | absolute position in seconds (float) | `OK seek SEC` |
| `seek +SEC` / `seek -SEC` | relative jump | `OK seek SEC (rel)` |
| `speed R` | tape-style speed, `0.25`–`4.0`: `1.0` normal, `<1` slower+lower, `>1` faster+higher (pitch coupled) | `OK speed R` |
| `gain G` | output level, `0.0`–`1.0` | `OK gain G` |
| `status` | current state | `state=… file=… pos=… dur=… speed=… gain=…` |
| `quit` | shut the app down | `OK quit` |

`status` fields: `state` ∈ `stopped|playing|paused`; `pos`/`dur` in seconds;
`speed` the tape ratio; `gain` 0–1. Example:
`state=playing file=/mnt/SD/track.mp3 pos=12.34 dur=210.5 speed=1.000 gain=1.00`.

### Examples

```sh
echo 'play /mnt/SD/track.mp3' > /tmp/varan/cmd     # fire-and-forget
printf 'status\n' | socat - UNIX-CONNECT:/tmp/varan/ctl
printf 'seek +30\n' | socat - UNIX-CONNECT:/tmp/varan/ctl
echo 'speed 0.85' > /tmp/varan/cmd                 # slow down (lower pitch)
printf 'gain 0.5\n'  | socat - UNIX-CONNECT:/tmp/varan/ctl
```

The UI also drives the engine through the same path: opening a file in the
browser (RIGHT on a regular file) issues `play <path>` internally.

## Planned (later slices)

| Command | Meaning |
|---|---|
| `loop START END` / `loop off` | loop between two marks (seconds) |
| `fx NAME PARAM VALUE` / `fx NAME on|off` | effect params, e.g. `fx reverb mix 0.3` |
| `next` / `prev` | playlist navigation |
| `subscribe` | stream async events (position ticks, track changes) |

## MIDI-CC mapping

MIDI is not a separate protocol: a mapper translates incoming CC messages into
these commands (a CC → `speed`, another → `seek +/-`, a note → `loop`/transport).
The mapping is configuration — users remap without rebuilding.

## Design notes

- **Text first.** Human-typeable over the serial console for debugging; a binary
  fast-path can come later for high-rate event streams.
- **One protocol, all clients.** First-party UI and third-party plugins are
  symmetric.
- **Stateless commands, stateful engine.** Clients resync with `status` rather
  than assuming.
