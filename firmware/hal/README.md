# firmware/hal/ — hardware abstraction layer

One thin module per peripheral, exposing a small, board-agnostic API to the
audio engine and the application above it. Ported and trimmed from the
prototype's already-working drivers.

Planned modules:

| Module | Hardware | Notes |
|---|---|---|
| `alsa` | V3s internal codec | playback out (+ capture for the mic, later) |
| `midi` | UART1 @ 31250 baud | needs the register prescaler (see `linux/root/uart_prescaler.sh`) |
| `oled` | SSD1306-class, I2C bus1 @0x3c | threaded driver + `Display` text/menu layer |
| `buttons` | GPIO buttons + rocker (up/down/press) | debounced edge events |
| `leds` | ~27 chip LEDs | ring/indicator patterns |
| `mpr121` | cap-touch, I2C0 @0x5a, 12 electrodes | touch ring |
| `lis3dh` | accelerometer, @0x19 | orientation / gesture (optional) |

The HAL stays free of player logic — it reports events and accepts commands;
policy lives above it. Pin and register details for each peripheral are in
[`../../docs/hardware.md`](../../docs/hardware.md).
