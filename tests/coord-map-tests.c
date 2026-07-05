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

/* Unit tests for the pure coordinate-mapping module. No OBS deps. */

#include <math.h>
#include <stdio.h>

#include "../src/coord-map.h"

static int failures = 0;

#define CHECK(cond)                                                             \
	do {                                                                    \
		if (!(cond)) {                                                  \
			printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
			failures++;                                             \
		}                                                               \
	} while (0)

static int near(double a, double b)
{
	return fabs(a - b) < 0.0001;
}

static struct ss_rect R(double x, double y, double w, double h)
{
	struct ss_rect r = {x, y, w, h};
	return r;
}

static struct ss_capture_geom G(double rx, double ry, double rw, double rh, double sw, double sh)
{
	struct ss_capture_geom g;
	g.screen_region = R(rx, ry, rw, rh);
	g.src_w = sw;
	g.src_h = sh;
	return g;
}

/* By-value wrapper so tests can pass temporaries (C forbids &rvalue). */
static enum ss_map_result map(const struct ss_capture_geom *g, struct ss_rect screen, double pad,
			      struct ss_rect *out)
{
	return ss_map_screen_rect(g, &screen, pad, out);
}

static void test_identity(void)
{
	/* Primary 1920x1080 monitor, unscaled capture, no padding. */
	struct ss_capture_geom g = G(0, 0, 1920, 1080, 1920, 1080);
	struct ss_rect out;
	CHECK(map(&g, R(100, 200, 300, 150), 0, &out) == SS_MAP_OK);
	CHECK(near(out.x, 100) && near(out.y, 200) && near(out.w, 300) && near(out.h, 150));
}

static void test_secondary_monitor_offset(void)
{
	/* Second monitor sits right of primary: region origin 1920,0. */
	struct ss_capture_geom g = G(1920, 0, 2560, 1440, 2560, 1440);
	struct ss_rect out;
	CHECK(map(&g, R(1920 + 500, 300, 200, 100), 0, &out) == SS_MAP_OK);
	CHECK(near(out.x, 500) && near(out.y, 300) && near(out.w, 200) && near(out.h, 100));
}

static void test_negative_origin_monitor(void)
{
	/* Monitor left of primary has negative screen x. */
	struct ss_capture_geom g = G(-1920, 0, 1920, 1080, 1920, 1080);
	struct ss_rect out;
	CHECK(map(&g, R(-1800, 50, 100, 40), 0, &out) == SS_MAP_OK);
	CHECK(near(out.x, 120) && near(out.y, 50));
}

static void test_mixed_dpi(void)
{
	/* Two monitors with different DPI scale: mapping only depends on
	 * each capture's own region/source ratio. */
	struct ss_capture_geom g100 = G(0, 0, 1920, 1080, 1920, 1080);    /* 100% */
	struct ss_capture_geom g150 = G(1920, 0, 2880, 1620, 2880, 1620); /* 150%, physical px */
	struct ss_rect out;
	CHECK(map(&g100, R(1520, 40, 360, 96), 0, &out) == SS_MAP_OK);
	CHECK(near(out.w, 360) && near(out.h, 96));
	CHECK(map(&g150, R(1920 + 2400, 60, 400, 120), 0, &out) == SS_MAP_OK);
	CHECK(near(out.x, 2400) && near(out.w, 400));
}

static void test_scaled_capture(void)
{
	/* Capture region 2560x1440 delivered as a 1280x720 source: source
	 * pixels are half-size, mapped rect must shrink by 2x. */
	struct ss_capture_geom g = G(0, 0, 2560, 1440, 1280, 720);
	struct ss_rect out;
	CHECK(map(&g, R(400, 200, 600, 300), 0, &out) == SS_MAP_OK);
	CHECK(near(out.x, 200) && near(out.y, 100) && near(out.w, 300) && near(out.h, 150));
}

static void test_partial_overlap_cropped(void)
{
	/* Rect hangs off the left edge of the captured region: only the
	 * intersecting part maps (a window capture is effectively a crop
	 * of the screen). */
	struct ss_capture_geom g = G(1000, 500, 800, 600, 800, 600);
	struct ss_rect out;
	CHECK(map(&g, R(900, 600, 300, 100), 0, &out) == SS_MAP_OK);
	CHECK(near(out.x, 0) && near(out.y, 100) && near(out.w, 200) && near(out.h, 100));
}

static void test_not_visible(void)
{
	struct ss_capture_geom g = G(0, 0, 1920, 1080, 1920, 1080);
	struct ss_rect out;
	CHECK(map(&g, R(3000, 3000, 100, 100), 0, &out) == SS_MAP_NOT_VISIBLE);
	/* Touching edge only (zero-area intersection) is not visible. */
	CHECK(map(&g, R(1920, 0, 100, 100), 0, &out) == SS_MAP_NOT_VISIBLE);
}

static void test_padding_expansion(void)
{
	struct ss_capture_geom g = G(0, 0, 1920, 1080, 1920, 1080);
	struct ss_rect out;
	CHECK(map(&g, R(500, 400, 200, 100), 12, &out) == SS_MAP_OK);
	CHECK(near(out.x, 488) && near(out.y, 388) && near(out.w, 224) && near(out.h, 124));
}

static void test_padding_clamped_at_edges(void)
{
	struct ss_capture_geom g = G(0, 0, 1920, 1080, 1920, 1080);
	struct ss_rect out;
	/* Rect at the very corner: padding must clamp, not go negative. */
	CHECK(map(&g, R(0, 0, 100, 50), 16, &out) == SS_MAP_OK);
	CHECK(near(out.x, 0) && near(out.y, 0) && near(out.w, 116) && near(out.h, 66));
	/* Rect at bottom-right: clamp against source bounds. */
	CHECK(map(&g, R(1820, 1030, 100, 50), 16, &out) == SS_MAP_OK);
	CHECK(near(out.x, 1804) && near(out.y, 1014));
	CHECK(near(out.x + out.w, 1920) && near(out.y + out.h, 1080));
}

static void test_padding_never_shrinks(void)
{
	/* Padded result must contain the unpadded result. */
	struct ss_capture_geom g = G(0, 0, 2560, 1440, 1280, 720);
	struct ss_rect a, b;
	CHECK(map(&g, R(100, 100, 50, 40), 0, &a) == SS_MAP_OK);
	CHECK(map(&g, R(100, 100, 50, 40), 8, &b) == SS_MAP_OK);
	CHECK(b.x <= a.x && b.y <= a.y);
	CHECK(b.x + b.w >= a.x + a.w && b.y + b.h >= a.y + a.h);
}

static void test_invalid_inputs(void)
{
	struct ss_rect out;
	struct ss_capture_geom g = G(0, 0, 1920, 1080, 1920, 1080);

	CHECK(map(NULL, R(0, 0, 1, 1), 0, &out) == SS_MAP_INVALID);
	CHECK(ss_map_screen_rect(&g, NULL, 0, &out) == SS_MAP_INVALID);

	struct ss_capture_geom zero_region = G(0, 0, 0, 1080, 1920, 1080);
	CHECK(map(&zero_region, R(0, 0, 1, 1), 0, &out) == SS_MAP_INVALID);

	struct ss_capture_geom zero_src = G(0, 0, 1920, 1080, 0, 0);
	CHECK(map(&zero_src, R(0, 0, 1, 1), 0, &out) == SS_MAP_INVALID);

	struct ss_capture_geom nan_geom = G(0, 0, NAN, 1080, 1920, 1080);
	CHECK(map(&nan_geom, R(0, 0, 1, 1), 0, &out) == SS_MAP_INVALID);

	CHECK(map(&g, R(0, 0, NAN, 1), 0, &out) == SS_MAP_INVALID);
	CHECK(map(&g, R(0, 0, -5, 1), 0, &out) == SS_MAP_INVALID);
	CHECK(map(&g, R(0, 0, 1, 1), -1, &out) == SS_MAP_INVALID);
}

int main(void)
{
	test_identity();
	test_secondary_monitor_offset();
	test_negative_origin_monitor();
	test_mixed_dpi();
	test_scaled_capture();
	test_partial_overlap_cropped();
	test_not_visible();
	test_padding_expansion();
	test_padding_clamped_at_edges();
	test_padding_never_shrinks();
	test_invalid_inputs();

	if (failures) {
		printf("coord-map-tests: %d FAILURE(S)\n", failures);
		return 1;
	}
	printf("coord-map-tests: all passed\n");
	return 0;
}
