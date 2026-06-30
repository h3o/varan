# firmware/protocol/ — the command socket

The control seam of the whole system. The UI, scripts, MIDI mapper, and
third-party plugins all drive the audio engine by sending commands over a local
socket (Unix domain socket / FIFO). One surface, three roles: scripting seam,
open/closed boundary, and plugin API.

The wire format and the full command set are specified in
[`../../docs/command-protocol.md`](../../docs/command-protocol.md). In short, a
line-oriented text command set, e.g.:

```
play /media/music/track.flac
pause
seek 90.0            # seconds, absolute
speed 0.92           # tape-style; 1.0 = normal
loop 4.0 8.0         # loop between two marks (seconds)
fx reverb mix 0.3
status               # → engine replies with current state
```

This directory will hold the protocol's reference implementation (server side in
the engine, plus a tiny client library) so scripts and plugins don't each
reinvent it.

_Spec lives in docs; reference code lands alongside the engine in Phase 2/4._
