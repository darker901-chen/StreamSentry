/*
StreamSentry
Copyright (C) 2026 darker hahaha901@gmail.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

/* Pure coordinate-mapping module. No OBS or Windows dependencies so it
 * can be unit-tested standalone (ctest).
 *
 * Coordinate chain (SPEC.md): UIA/Win32 screen coords (physical pixels,
 * virtual-screen space, origins may be negative on multi-monitor setups)
 * -> capture-source pixel space. Scene-item transforms (scale/crop that
 * the user applies in OBS) are applied by OBS downstream of the filter,
 * so a rect correct in source space stays glued to the content under
 * those transforms.
 */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ss_rect {
	double x;
	double y;
	double w;
	double h;
};

/* What the capture source shows and how big its output is.
 * screen_region: the virtual-screen rectangle captured (a monitor rect
 * for display capture, a window frame rect for window capture), in
 * physical pixels. src_w/src_h: the source's pixel dimensions. When the
 * source is unscaled these match screen_region.w/h; a mismatch means
 * the capture is scaled and mapping must rescale accordingly. */
struct ss_capture_geom {
	struct ss_rect screen_region;
	double src_w;
	double src_h;
};

enum ss_map_result {
	SS_MAP_OK = 0,      /* out contains a clamped source-space rect */
	SS_MAP_NOT_VISIBLE, /* rect does not intersect the captured region */
	SS_MAP_INVALID,     /* inputs unusable -> caller must fail closed */
};

/* Map a screen-space rect into source space, expand it by pad_px source
 * pixels on every side (over-mask, never under-mask), and clamp to the
 * source bounds. Any non-finite or degenerate geometry input yields
 * SS_MAP_INVALID; per the iron rules the caller must treat that as
 * fail-closed, not as "no rect". */
enum ss_map_result ss_map_screen_rect(const struct ss_capture_geom *geom, const struct ss_rect *screen_rect,
				      double pad_px, struct ss_rect *out);

#ifdef __cplusplus
}
#endif
