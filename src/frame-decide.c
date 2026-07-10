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

#include "frame-decide.h"

static void flag(struct ss_frame_decision *out, const char *reason)
{
	if (!out->unverified) {
		out->unverified = true;
		out->reason = reason;
	}
	/* already flagged: first trigger keeps the reason */
}

void ss_decide_frame(const struct ss_snapshot *snap, bool have_snap, uint64_t age_ns, uint64_t stale_ns,
		     bool geom_valid, const struct ss_capture_geom *geom, double pad_px,
		     struct ss_frame_decision *out)
{
	out->unverified = false;
	out->reason = "";
	out->num_mapped = 0;

	if (!snap || !have_snap) {
		flag(out, "no detection snapshot");
		return;
	}
	if (age_ns > stale_ns) {
		/* Stale data is untrusted: no masks drawn from it. */
		flag(out, "heartbeat stale");
		return;
	}

	/* Watcher-side degradation (e.g. toast gate blind): the chip must
	 * show, but confident masks below are NOT gated by this — that was
	 * spec-guardian M6.5 finding V6. */
	if (snap->detection_degraded)
		flag(out, "detection degraded (monitor data unavailable)");

	if (snap->num_rects == 0)
		return;

	if (!geom_valid || !geom) {
		flag(out, "capture geometry unresolved (unsupported source or scaled capture)");
		return;
	}

	for (size_t i = 0; i < snap->num_rects && i < SS_MAX_RECTS; i++) {
		struct ss_rect mapped;
		enum ss_map_result r = ss_map_screen_rect(geom, &snap->rects[i].screen, pad_px, &mapped);
		if (r == SS_MAP_OK) {
			out->mapped[out->num_mapped] = mapped;
			out->kinds[out->num_mapped] = snap->rects[i].kind;
			out->num_mapped++;
		} else if (r == SS_MAP_INVALID) {
			flag(out, "coordinate mapping failed for a detection");
		}
		/* NOT_VISIBLE: outside this capture, skip silently */
	}
}
