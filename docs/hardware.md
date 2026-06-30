# Hardware map

Consolidated reference for writing firmware against Pocket Varan. The
authoritative sources are the device tree
([`../linux/device_tree/sun8i-v3s-vr294.dts`](../linux/device_tree/sun8i-v3s-vr294.dts))
and the init/helper scripts under [`../linux/root/`](../linux/root/) and
[`../linux/init.d/`](../linux/init.d/); this page summarizes them. Where a value
isn't pinned down in those files (evolving prototype wiring), it's flagged.

## SoC & memory

- **Allwinner V3s** (`sun8i-v3s`), single ARM Cortex-A7 ~1.2 GHz, NEON/VFPv4,
  hard-float (`arm-linux-gnueabihf`).
- **64 MB on-die DDR2** (~55 MB usable) — the dominant design constraint.
- Linux 4.14.x (LicheePi-Zero lineage), Buildroot userspace.
- Board compatible string: `licheepi,licheepi-zero-dock` / `…-zero` /
  `allwinner,sun8i-v3s`.

## Storage

**SPI-NOR, 32 MB** (`spi0`), partition map from the DTS:

| Partition | Offset | Size |
|---|---|---|
| `u-boot` | `0x000000` | 1 MB |
| `dtb` | `0x100000` | 64 KB |
| `kernel` | `0x110000` | 4 MB |
| `rootfs` | `0x510000` | ~11 MB (`0xaf0000`) |
| `samples` | `0x1000000` | 16 MB |

**microSD** via `mmc0` (PF0–PF5) — bulk media storage (`mmcblk0`).

## Audio

V3s internal codec (`allwinner,sun8i-v3s-codec` + `…-codec-analog`).
Audio routing: `Headphone`→`HP`/`HPCOM` (headphone out) and `MIC1`←`Mic` with
`HBIAS` (onboard electret mic). Driven via ALSA from userspace.

## I²C buses & devices

| Bus | Pins | Device | Addr | Notes |
|---|---|---|---|---|
| i2c0 | PB6 / PB7 | MPR121 cap-touch (12 electrodes) | `0x5a` | init in userspace — `root/init_mpr121_0x5a_i2c0.sh` |
| i2c0 | PB6 / PB7 | LIS3DH accelerometer | `0x19` | optional orientation/gesture |
| i2c1 | PB8 / PB9 | SSD1306-class OLED | `0x3c` | threaded driver |
| (i2c) | — | NS2009 resistive touch | `0x48` | `nsiway,ns2009`, present in DTS |

> The OLED (i2c1) and MIDI (uart1) can contend for **PE21/PE22** in alternate
> wirings — the DTS carries both pin groups with comments ("enable this for OLED
> *instead of* MIDI I/O" and vice-versa). The current build puts OLED on
> PB8/PB9 and gives PE21/PE22 to MIDI.

## MIDI

- **UART1** on **PE21 / PE22**, exposed as **`/dev/ttyS0`** (uart0 is disabled,
  so uart1 enumerates first), on 3.5 mm jacks.
- Standard MIDI **31250 baud** is set with a register prescaler hack
  (`root/uart_prescaler.sh`): `devmem 0x01c2840c 32 0x93; devmem 0x01c28400 32
  0x30; devmem 0x01c2840c 32 0x13`.
- `root/midi_init.sh` frees the UART from `getty`, applies the prescaler, and
  sends a test note.

## Controls

**GPIO buttons** (current "v2" wiring — `root/init_buttons_v2.sh`,
`root/read_buttons_v2.sh`):

| Button | Pin | gpio# |
|---|---|---|
| Power | PB3 | 35 |
| Center | PE19 | 147 |
| Top-middle | PE23 | 151 |
| Top-left | PB2 | 34 |
| Top-right | PG1 | 193 |

Pull-ups/downs are applied via `devmem` in the init script. A **rocker switch**
(up / down / press) and a **soft-shutdown** line also exist on the prototype;
their exact pins live in the board notes and are still settling, so confirm
against the current scripts before relying on them.

**LRADC keys** (`sun4i-a10-lradc-keys`, resistor-ladder on the LRADC): Volume Up,
Volume Down, Select, Start.

**LEDs:** ~27 chip LEDs driven from firmware (separate from the dock's
blue/green/red `licheepi:*:usr` LEDs in the DTS).

## USB

`sun8i-h3-musb` OTG controller (+ EHCI/OHCI). The product uses the port as a
**USB-C gadget**: CDC-ACM **serial console** + **mass-storage** exposing the
media partition.

## Power

1500–2000 mAh LiPo; the device draws ~120–150 mA depending on CPU load and how
many LEDs / OLED pixels are lit.
