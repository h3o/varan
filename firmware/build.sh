#!/bin/bash
# SPDX-License-Identifier: MIT
# Build the Pocket Varan navigate-only firmware for the V3s (VR294) board.
#
# Cross-compiles with the Linaro GCC 5.5 toolchain (same one the prototype used;
# it plays nicer with the 4.14 / glibc target than buildroot's). Override the
# toolchain prefix with TOOLCHAIN=/path/to/arm-linux-gnueabihf- ./build.sh
set -euo pipefail

cd "$(dirname "$0")"

TOOLCHAIN="${TOOLCHAIN:-/home/vagrant/orangepi-build/toolchains/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-}"
CXX="${TOOLCHAIN}g++"

if ! command -v "$CXX" >/dev/null 2>&1; then
  echo "error: C++ cross-compiler not found: $CXX" >&2
  echo "set TOOLCHAIN=/path/to/arm-linux-gnueabihf-  (note trailing dash)" >&2
  exit 1
fi

OUT_DIR="build"
OUT="$OUT_DIR/varan"
mkdir -p "$OUT_DIR"

# Everything is compiled as C++ (the prototype does the same; that's why the .c
# drivers link against the C++ TUs without extern "C").
SRC=(
  app/main.cpp
  app/varan_ui.cpp
  hal/hal.c
  hal/i2c.c
  hal/display.cpp
  hal/oled/ssd1306.c
  hal/oled/GUI/GUI_Paint.c
  hal/oled/Fonts/font8.c
  hal/oled/Fonts/font12.c
  hal/oled/Fonts/font16.c
  hal/oled/Fonts/font20.c
  hal/oled/Fonts/font24.c
)

INCLUDES=(-I. -Ihal -Ihal/oled)
DEFINES=(-DUSE_SSD1306 -DBOARD_VR291)
CXXFLAGS=(-O2 -ffast-math -std=gnu++11 -Wno-write-strings -g)
LIBS=(-lpthread -lstdc++fs -lm -lrt)

echo "CXX: $CXX"
echo "Building $OUT ..."
"$CXX" "${CXXFLAGS[@]}" "${DEFINES[@]}" "${INCLUDES[@]}" "${SRC[@]}" -o "$OUT" "${LIBS[@]}"

echo "Built: $OUT"
"${TOOLCHAIN}size" "$OUT" 2>/dev/null || true
file "$OUT" 2>/dev/null || true
