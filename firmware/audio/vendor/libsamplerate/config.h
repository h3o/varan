/* Minimal hand-written config for the vendored libsamplerate (0.2.2).
 *
 * Upstream generates this via CMake; we only need a handful of macros. We build
 * just the FAST sinc converter (fastest_coeffs.h, ~72 KB) — the MEDIUM/BEST
 * tables (0.6 MB / 9 MB) are dropped to keep the firmware and repo small.
 * Compiled with the C compiler (see firmware/build.sh); these sources are not
 * C++-clean. SPDX-License-Identifier: BSD-2-Clause (see COPYING). */

#ifndef VARAN_LIBSAMPLERATE_CONFIG_H
#define VARAN_LIBSAMPLERATE_CONFIG_H

#define PACKAGE "libsamplerate"
#define VERSION "0.2.2"

#define HAVE_STDBOOL_H 1

/* Only the fast sinc converter is compiled in (SRC_SINC_FASTEST). */
#define ENABLE_SINC_FAST_CONVERTER 1

/* ARM does not saturate on float->int cast, so let libsamplerate clip. */
#define CPU_CLIPS_POSITIVE 0
#define CPU_CLIPS_NEGATIVE 0

#endif /* VARAN_LIBSAMPLERATE_CONFIG_H */
