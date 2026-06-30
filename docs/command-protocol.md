# Command protocol

> **Draft.** This documents the *intended* shape of the control seam. It will
> firm up as the player core is implemented; treat specifics as provisional.

The engine exposes a local **command socket** (Unix domain socket; a FIFO pair is
the fallback). Clients — the UI, the MIDI-CC mapper, shell scripts, and plugins —
send line-oriented text commands and receive line-oriented replies/events.

## Transport & playback

| Command | Meaning |
|---|---|
| `play [PATH]` | start/resume; with a path, load and play it |
| `pause` | pause, keep position |
| `stop` | stop, release the file |
| `next` / `prev` | playlist navigation |
| `seek SECONDS` | absolute position, in seconds (float) |
| `seek +SECONDS` / `seek -SECONDS` | relative jump |

## Speed / "tuning"

| Command | Meaning |
|---|---|
| `speed RATIO` | tape-style; `1.0` = normal, `0.92` slower+lower, `1.05` faster+higher |

(Pitch-preserving stretch is a later addition and would get its own command.)

## Loop slices

| Command | Meaning |
|---|---|
| `loop START END` | loop between two marks (seconds) |
| `loop off` | clear the loop |

## Effects

| Command | Meaning |
|---|---|
| `fx NAME PARAM VALUE` | set an effect parameter, e.g. `fx reverb mix 0.3` |
| `fx NAME on` / `fx NAME off` | enable/disable an effect |

## Query & events

| Command | Meaning |
|---|---|
| `status` | reply with current file, position, speed, loop, fx state |
| `subscribe` | stream asynchronous events (position ticks, track changes, input) |

## MIDI-CC mapping

MIDI is not a separate protocol: a mapper process translates incoming CC messages
into these commands (e.g. a CC → `speed`, another CC → `seek +/-`, a note →
`loop`/transport). The mapping is configuration, so users can remap without
rebuilding anything.

## Design notes

- **Text first.** Human-typeable over the USB serial console for debugging; a
  binary fast-path can be added later for high-rate event streams if needed.
- **One socket, all clients.** First-party UI and third-party plugins are
  symmetric — whatever the UI can do, a plugin can do.
- **Stateless commands, stateful engine.** Commands mutate engine state; clients
  resync with `status`/`subscribe` rather than assuming.
