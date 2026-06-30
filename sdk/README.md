# sdk/ — plugin SDK

How to extend Pocket Varan without touching the closed application.

A plugin is anything that speaks the **command-socket protocol**
([`../firmware/protocol/`](../firmware/protocol/)): it can issue transport/speed/
loop/FX commands and subscribe to engine state and input events (buttons, rocker,
touch, MIDI). Because that socket is the same boundary the shipping app uses, a
plugin is a first-class citizen, not a second-class hook.

Planned contents:

- protocol client headers (C) + a minimal language-agnostic description,
- an **example plugin** (likely a small effect or a custom MIDI-CC mapping) that
  builds and runs on-device,
- packaging/placement conventions so the device can discover and launch plugins.

This is the heart of the open-core promise: an unlocked, documented, expandable
device. The SDK is intentionally public and MIT-licensed.

_Empty for now — populated in Phase 4 (productize + open the community layer),
once the protocol has stabilized against the real player core._
