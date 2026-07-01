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
mkdir -p "$OUT_DIR"

# Resampler quality: SRC_QUALITY=fast (default) or medium. Medium also compiles
# in libsamplerate's mid_qual sinc table and writes a separate build/varan-medium
# so the two can be A/B compared.
SRC_QUALITY="${SRC_QUALITY:-fast}"
LSR_QUAL_DEF=()
SRC_QUAL_DEF=()
OUT="$OUT_DIR/varan"
if [ "$SRC_QUALITY" = "medium" ]; then
  LSR_QUAL_DEF=(-DENABLE_SINC_MEDIUM_CONVERTER)
  SRC_QUAL_DEF=(-DVARAN_SRC_MEDIUM)
  OUT="$OUT_DIR/varan-medium"
fi

# Everything is compiled as C++ (the prototype does the same; that's why the .c
# drivers link against the C++ TUs without extern "C").
SRC=(
  app/main.cpp
  app/varan_ui.cpp
  hal/hal.c
  hal/i2c.c
  hal/keys.c
  hal/leds.c
  hal/display.cpp
  hal/oled/ssd1306.c
  hal/oled/GUI/GUI_Paint.c
  hal/oled/Fonts/font8.c
  hal/oled/Fonts/font12.c
  hal/oled/Fonts/font16.c
  hal/oled/Fonts/font20.c
  hal/oled/Fonts/font24.c
  protocol/command.c
  audio/pcm_alsa.c
  audio/resampler.c
  audio/engine.c
  audio/cmd_listener.c
  audio/minimp3_impl.c
)

# ALSA lives in the prototype's cross prefix (/usr/v3slib); minimp3 + libsamplerate
# are vendored under audio/vendor/.
ALSA_PREFIX="${ALSA_PREFIX:-/usr/v3slib}"
LSR_DIR="audio/vendor/libsamplerate"
INCLUDES=(-I. -Ihal -Ihal/oled -Iaudio -Iprotocol -Iaudio/vendor/minimp3 "-I${LSR_DIR}" "-I${ALSA_PREFIX}/include")
# SSD1306_ROTATE = 180° flip (COM scan + column order), matching how the OLED is
# mounted on the production VR29x board. Drop it if a board mounts the panel the
# other way up.
DEFINES=(-DUSE_SSD1306 -DBOARD_VR291 -DSSD1306_ROTATE)
CXXFLAGS=(-O2 -ffast-math -std=gnu++11 -Wno-write-strings -g)
LIBS=("-L${ALSA_PREFIX}/lib" -lasound -lpthread -lstdc++fs -lm -lrt -ldl)

# libsamplerate is plain C (not C++-clean), so compile its TUs with the C
# compiler and only the FAST sinc converter (see its config.h), then link the
# objects into the C++ build.
CC="${TOOLCHAIN}gcc"
LSR_SRC=(samplerate.c src_linear.c src_sinc.c src_zoh.c)
LSR_OBJS=()
for s in "${LSR_SRC[@]}"; do
  obj="$OUT_DIR/lsr_${s%.c}.o"
  echo "CC  $LSR_DIR/$s"
  "$CC" -O2 -ffast-math -std=gnu99 -DHAVE_CONFIG_H "${LSR_QUAL_DEF[@]}" "-I${LSR_DIR}" -c "$LSR_DIR/$s" -o "$obj"
  LSR_OBJS+=("$obj")
done

echo "CXX: $CXX  (SRC_QUALITY=$SRC_QUALITY)"
echo "Building $OUT ..."
"$CXX" "${CXXFLAGS[@]}" "${DEFINES[@]}" "${SRC_QUAL_DEF[@]}" "${INCLUDES[@]}" "${SRC[@]}" "${LSR_OBJS[@]}" -o "$OUT" "${LIBS[@]}"

echo "Built: $OUT"
"${TOOLCHAIN}size" "$OUT" 2>/dev/null || true
file "$OUT" 2>/dev/null || true
