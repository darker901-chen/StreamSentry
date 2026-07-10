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

/* Unit tests for the pure per-frame masking decision (SPEC 2.7
 * ordering rules). The headline case is the spec-guardian M6.5 V6
 * regression: detection_degraded must show the chip WITHOUT dropping
 * confidently mapped masks. */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/frame-decide.h"

static int failures = 0;

#define CHECK(cond)                                                             \
	do {                                                                    \
		if (!(cond)) {                                                  \
			printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
			failures++;                                             \
		}                                                               \
	} while (0)

#define STALE_NS 500000000ULL
#define PAD 12.0

static struct ss_capture_geom geom_1080(void)
{
	struct ss_capture_geom g;
	g.screen_region.x = 0;
	g.screen_region.y = 0;
	g.screen_region.w = 1920;
	g.screen_region.h = 1080;
	g.src_w = 1920;
	g.src_h = 1080;
	return g;
}

static struct ss_snapshot snap_with_rect(enum ss_rect_kind kind, double x, double y, double w, double h)
{
	struct ss_snapshot s;
	memset(&s, 0, sizeof(s));
	s.num_rects = 1;
	s.rects[0].kind = kind;
	s.rects[0].screen.x = x;
	s.rects[0].screen.y = y;
	s.rects[0].screen.w = w;
	s.rects[0].screen.h = h;
	return s;
}

int main(void)
{
	struct ss_capture_geom geom = geom_1080();
	struct ss_frame_decision d;

	/* healthy, one confident blocklist rect: masked, no chip */
	{
		struct ss_snapshot s = snap_with_rect(SS_RECT_WINDOW, 100, 100, 400, 300);
		ss_decide_frame(&s, true, 0, STALE_NS, true, &geom, PAD, &d);
		CHECK(!d.unverified);
		CHECK(d.num_mapped == 1);
		CHECK(d.kinds[0] == SS_RECT_WINDOW);
	}

	/* V6 REGRESSION: detection_degraded + confident rect ->
	 * chip AND the mask both present */
	{
		struct ss_snapshot s = snap_with_rect(SS_RECT_WINDOW, 100, 100, 400, 300);
		s.detection_degraded = true;
		ss_decide_frame(&s, true, 0, STALE_NS, true, &geom, PAD, &d);
		CHECK(d.unverified);
		CHECK(strcmp(d.reason, "detection degraded (monitor data unavailable)") == 0);
		CHECK(d.num_mapped == 1); /* the confident mask is NOT dropped */
	}

	/* degraded with zero rects: chip still shows (a toast could be
	 * getting missed), nothing to mask */
	{
		struct ss_snapshot s;
		memset(&s, 0, sizeof(s));
		s.detection_degraded = true;
		ss_decide_frame(&s, true, 0, STALE_NS, true, &geom, PAD, &d);
		CHECK(d.unverified);
		CHECK(d.num_mapped == 0);
	}

	/* no snapshot ever: chip, no masks */
	{
		ss_decide_frame(NULL, false, UINT64_MAX, STALE_NS, false, NULL, PAD, &d);
		CHECK(d.unverified);
		CHECK(strcmp(d.reason, "no detection snapshot") == 0);
		CHECK(d.num_mapped == 0);
	}

	/* stale heartbeat: stale rects are NOT drawn */
	{
		struct ss_snapshot s = snap_with_rect(SS_RECT_WINDOW, 100, 100, 400, 300);
		ss_decide_frame(&s, true, STALE_NS + 1, STALE_NS, true, &geom, PAD, &d);
		CHECK(d.unverified);
		CHECK(strcmp(d.reason, "heartbeat stale") == 0);
		CHECK(d.num_mapped == 0);
	}

	/* rects pending but geometry unresolved: chip, no masks */
	{
		struct ss_snapshot s = snap_with_rect(SS_RECT_TOAST, 1500, 30, 396, 180);
		ss_decide_frame(&s, true, 0, STALE_NS, false, NULL, PAD, &d);
		CHECK(d.unverified);
		CHECK(strcmp(d.reason, "capture geometry unresolved (unsupported source or scaled capture)") == 0);
		CHECK(d.num_mapped == 0);
	}

	/* one INVALID rect among confident ones: confident stays masked,
	 * frame flagged */
	{
		struct ss_snapshot s;
		memset(&s, 0, sizeof(s));
		s.num_rects = 2;
		s.rects[0].kind = SS_RECT_WINDOW;
		s.rects[0].screen.x = 100;
		s.rects[0].screen.y = 100;
		s.rects[0].screen.w = 400;
		s.rects[0].screen.h = 300;
		s.rects[1].kind = SS_RECT_FIELD;
		s.rects[1].screen.x = 50;
		s.rects[1].screen.y = 50;
		s.rects[1].screen.w = -5; /* degenerate -> SS_MAP_INVALID */
		s.rects[1].screen.h = 40;
		ss_decide_frame(&s, true, 0, STALE_NS, true, &geom, PAD, &d);
		CHECK(d.unverified);
		CHECK(strcmp(d.reason, "coordinate mapping failed for a detection") == 0);
		CHECK(d.num_mapped == 1);
		CHECK(d.kinds[0] == SS_RECT_WINDOW);
	}

	/* off-capture rect: skipped silently, no chip */
	{
		struct ss_snapshot s = snap_with_rect(SS_RECT_WINDOW, 5000, 100, 400, 300);
		ss_decide_frame(&s, true, 0, STALE_NS, true, &geom, PAD, &d);
		CHECK(!d.unverified);
		CHECK(d.num_mapped == 0);
	}

	/* reason precedence: degraded (#3) wins over mapping failure (#5) */
	{
		struct ss_snapshot s = snap_with_rect(SS_RECT_WINDOW, 50, 50, -5, 40);
		s.detection_degraded = true;
		ss_decide_frame(&s, true, 0, STALE_NS, true, &geom, PAD, &d);
		CHECK(d.unverified);
		CHECK(strcmp(d.reason, "detection degraded (monitor data unavailable)") == 0);
	}

	if (failures) {
		printf("frame-decide-tests: %d FAILURE(S)\n", failures);
		return 1;
	}
	printf("frame-decide-tests: all passed\n");
	return 0;
}
