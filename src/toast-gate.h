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

/* Toast geometry gate (M6, SPEC 2.2). Pure module — no OBS, no Windows
 * — so the narrowing rule is unit-testable (ctest) like coord-map.
 *
 * Applied by the watcher AFTER the process+class toast signature: a
 * signature-matched window is reported as a toast only if its geometry
 * is plausible for a toast banner; otherwise it falls through to
 * ordinary block/allowlist matching. Per the 2026-07-09 owner ruling
 * (SPEC 2.7, mask only on confidence) the gate fails toward NOT
 * masking: inputs it cannot reason about classify as not-a-toast.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "coord-map.h"

#ifdef __cplusplus
extern "C" {
#endif

/* True when `win` (virtual-screen physical pixels) is geometrically
 * plausible as a toast banner on monitor `mon`. Degenerate/non-finite
 * inputs -> false (cannot affirm -> no toast card). */
bool ss_toast_geom_plausible(const struct ss_rect *mon, const struct ss_rect *win);

/* Gate against a monitor list: plausible on ANY monitor -> true.
 * Empty/NULL monitor list -> false (cannot affirm -> no toast card). */
bool ss_toast_geom_plausible_any(const struct ss_rect *mons, size_t num_mons, const struct ss_rect *win);

#ifdef __cplusplus
}
#endif
