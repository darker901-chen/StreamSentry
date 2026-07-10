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

/* Pure per-frame masking decision. No OBS, no Windows — extracted from
 * filter.c so the fail-open ordering rules (SPEC 2.7) are enforced by
 * unit tests, not just review (spec-guardian M6.5 finding V6: a
 * degradation flag must never gate confident masks).
 *
 * Semantics, in trigger order (ARCHITECTURE.md failure table):
 *  - unhealthy (no snapshot / stale heartbeat): no masks at all (stale
 *    rects are untrusted), frame unverified.
 *  - mode transition (snapshot produced under the other mode): rects
 *    mean the opposite thing — untrusted, frame unverified (one-tick
 *    transient after a mode switch).
 *  - detection_degraded (fresh heartbeat): frame unverified, but every
 *    confidently mapped rect IS still masked.
 *  - watcher mask_all (rect budget overflow): allowlist mode -> whole
 *    source masked (the mode's default, NOT a failure); blocklist mode
 *    -> the rects we do have are masked + frame unverified (dropped
 *    rects are never silent).
 *  - geometry unresolved while rects pending: no masks, unverified.
 *  - per-rect mapping: OK -> mask; NOT_VISIBLE -> skip silently;
 *    INVALID -> that rect unmasked + frame unverified (confident rects
 *    in the same frame keep their masks).
 * Allowlist failure direction (M7, SPEC 2.3+2.7): whenever the frame
 * is unverified for ANY reason — including detection_degraded —
 * allowlist mode masks the whole source; default-deny is what that
 * user opted into, and it must not depend on which processes they
 * approved (an approved toast host would otherwise show a real toast
 * during degradation — guardian M7 Q1).
 * The reason string reports the FIRST trigger in the order above. */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "coord-map.h"
#include "shared-state.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ss_frame_decision {
	bool unverified;    /* frame must show the status chip */
	const char *reason; /* static string; valid when unverified */
	bool mask_all;      /* draw one full-source privacy plate (and skip
	                     * per-rect plates; target is never composed) */
	size_t num_mapped;
	struct ss_rect mapped[SS_MAX_RECTS];
	enum ss_rect_kind kinds[SS_MAX_RECTS];
};

/* age_ns: heartbeat age (UINT64_MAX when unknown). geom_valid/geom:
 * capture geometry resolution result; consulted only when the (fresh)
 * snapshot carries rects. allowlist_mode: the FILTER's configured mode
 * (authoritative for failure direction; compared against the
 * snapshot's mode to catch switch transients). */
void ss_decide_frame(const struct ss_snapshot *snap, bool have_snap, uint64_t age_ns, uint64_t stale_ns,
		     bool allowlist_mode, bool geom_valid, const struct ss_capture_geom *geom, double pad_px,
		     struct ss_frame_decision *out);

#ifdef __cplusplus
}
#endif
