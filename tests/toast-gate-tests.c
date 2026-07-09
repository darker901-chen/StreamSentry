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

/* Unit tests for the pure toast-geometry gate. No OBS deps.
 *
 * Positive fixtures = documented Windows toast metrics (396 DIP wide,
 * right-edge anchored) at several DPI scales; negative fixtures = the
 * flyover/popup shapes recorded on the dev machine 2026-07-09
 * (reports/M6-toast-probe.txt) transplanted onto the toast signature.
 * The gate must fail toward masking on any degenerate input. */

#include <math.h>
#include <stdio.h>

#include "../src/toast-gate.h"

static int failures = 0;

#define CHECK(cond)                                                             \
	do {                                                                    \
		if (!(cond)) {                                                  \
			printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
			failures++;                                             \
		}                                                               \
	} while (0)

static struct ss_rect R(double x, double y, double w, double h)
{
	struct ss_rect r = {x, y, w, h};
	return r;
}

static bool any(struct ss_rect mon, struct ss_rect win)
{
	return ss_toast_geom_plausible_any(&mon, 1, &win);
}

int main(void)
{
	const struct ss_rect mon = R(0, 0, 2560, 1440);       /* dev machine, 100% */
	const struct ss_rect mon4k = R(0, 0, 3840, 2160);     /* 4K */
	const struct ss_rect mon2 = R(2560, -240, 1920, 1080); /* secondary, negative origin */

	/* ---- real toast shapes pass (documented metrics) ---- */
	/* 100%: 396x180 banner, bottom-right above taskbar, 16px inset */
	CHECK(any(mon, R(2560 - 16 - 396, 1392 - 16 - 180, 396, 180)));
	/* 100%: collapsed one-liner (79px tall), top-right anchor variant */
	CHECK(any(mon, R(2560 - 16 - 396, 16, 396, 79)));
	/* 150% on 4K: 594x270 */
	CHECK(any(mon4k, R(3840 - 24 - 594, 2112 - 24 - 270, 594, 270)));
	/* 200% on 4K: 792x360 expanded (tall) */
	CHECK(any(mon4k, R(3840 - 32 - 792, 2112 - 32 - 720, 792, 720)));
	/* slide-in animation: right edge 60px past the monitor edge */
	CHECK(any(mon, R(2560 - 396 + 60, 1100, 396, 180)));
	/* secondary monitor with negative-origin coords */
	CHECK(any(mon2, R(2560 + 1920 - 16 - 396, -240 + 700, 396, 160)));

	/* ---- flyover shapes are rejected (negative fixtures) ---- */
	/* Start search: huge, horizontally centered */
	CHECK(!any(mon, R(760, 120, 1040, 660)));
	/* taskbar hover preview: bottom-center */
	CHECK(!any(mon, R(1100, 1180, 300, 190)));
	/* small tooltip near the tray (right edge, but tiny) */
	CHECK(!any(mon, R(2380, 1350, 170, 36)));
	/* tray-overflow-shaped: right edge, 234x154 -> H fits but W < min?
	 * 234 > 200 so width passes; height 154 passes; edge passes: this
	 * SHAPE is admitted by design (over-mask-safe) — assert the gate's
	 * documented behavior so a future tightening is a conscious act. */
	CHECK(any(mon, R(2212, 1238, 234, 154)));
	/* full work-area surface (task switcher) */
	CHECK(!any(mon, R(0, 0, 2560, 1392)));
	/* full-width bottom bar */
	CHECK(!any(mon, R(0, 1340, 2560, 52)));
	/* left-edge popup of plausible toast size */
	CHECK(!any(mon, R(16, 400, 396, 180)));
	/* horizontally centered popup of plausible toast size */
	CHECK(!any(mon, R(1080, 400, 396, 180)));
	/* right-edge but taller than 90% of the monitor */
	CHECK(!any(mon, R(2100, 40, 440, 1360)));
	/* window on a different monitor than the one evaluated */
	{
		struct ss_rect mons1[1] = {R(0, 0, 2560, 1440)};
		struct ss_rect toast_on_mon2 = R(2560 + 1920 - 412, 700, 396, 180);
		CHECK(!ss_toast_geom_plausible_any(mons1, 1, &toast_on_mon2));
	}

	/* ---- multi-monitor: plausible on ANY monitor passes ---- */
	{
		struct ss_rect mons[2] = {R(0, 0, 2560, 1440), R(2560, -240, 1920, 1080)};
		struct ss_rect toast2 = R(2560 + 1920 - 412, -240 + 700, 396, 160);
		CHECK(ss_toast_geom_plausible_any(mons, 2, &toast2));
		struct ss_rect centered1 = R(1080, 400, 396, 180);
		CHECK(!ss_toast_geom_plausible_any(mons, 2, &centered1));
	}

	/* ---- uncertainty fails toward masking ---- */
	CHECK(ss_toast_geom_plausible_any(NULL, 0, &mon));               /* no monitors */
	{
		struct ss_rect mons[1] = {R(0, 0, 2560, 1440)};
		CHECK(ss_toast_geom_plausible_any(mons, 0, &mons[0]));   /* zero count */
	}
	CHECK(ss_toast_geom_plausible(NULL, &mon));                      /* null monitor */
	{
		struct ss_rect degenerate_mon = R(0, 0, 0, 0);
		struct ss_rect w = R(2200, 400, 396, 180);
		CHECK(ss_toast_geom_plausible(&degenerate_mon, &w));
	}
	{
		struct ss_rect nan_mon = R(nan(""), 0, 2560, 1440);
		struct ss_rect w = R(1080, 400, 396, 180); /* would be rejected on a sane monitor */
		CHECK(ss_toast_geom_plausible(&nan_mon, &w));
	}

	if (failures) {
		printf("toast-gate-tests: %d FAILURE(S)\n", failures);
		return 1;
	}
	printf("toast-gate-tests: all passed\n");
	return 0;
}
