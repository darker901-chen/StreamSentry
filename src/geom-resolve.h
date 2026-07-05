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
 * M2 scope: display/monitor capture only. Window/game/other capture
 * types cannot have their region resolved confidently here, so the
 * resolver reports failure and the filter fails closed (iron rule 1:
 * never guess a mask position). */

#pragma once

#include <obs-module.h>

#include "coord-map.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Resolve the capture geometry for a filter target. Returns true and
 * fills *out only when the geometry is known with confidence (the
 * candidate monitor's pixel size matches the source's base size).
 * Returns false whenever anything is uncertain — the caller must then
 * fail closed if there are rects to mask. */
bool ss_resolve_capture_geom(obs_source_t *target, struct ss_capture_geom *out);

#ifdef __cplusplus
}
#endif
