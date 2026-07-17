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

/* Filter-side capture-geometry resolution: figure out which virtual-
 * screen region the filter's target source is capturing, so watcher
 * screen-space rects can be mapped into source space.
 *
 * Supported with confidence:
 * - Display/monitor capture when exactly one monitor matches the source size.
 * - Windows Window Capture when OBS reports an active hooked window, that
 *   identity resolves to exactly one HWND, and exactly one client/full-frame
 *   rectangle matches the source base size.
 *
 * Any ambiguity reports failure so the caller renders DEGRADED with the
 * status chip (SPEC 2.7: never guess a mask position, never fail silently). */

#pragma once

#include <obs-module.h>

#include "coord-map.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Resolve the capture geometry for a filter target. Returns true and fills
 * *out only when the source type, OS window/monitor identity, origin, and
 * physical-pixel dimensions are all known with confidence. Returns false for
 * unsupported sources, duplicate window identities, ambiguous monitor/rect
 * matches, minimized/unhooked windows, or any API failure. */
bool ss_resolve_capture_geom(obs_source_t *target, struct ss_capture_geom *out);

#ifdef __cplusplus
}
#endif
