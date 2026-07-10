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

/* Unit tests for the plate generator. The point of these tests is iron
 * rule 3: masking is OPAQUE. Opacity is asserted pixel-by-pixel here so
 * a regression cannot slip through as "looks fine on stream". */

#include <stdio.h>

#include "../src/plate-gen.h"

static int failures = 0;

#define CHECK(cond)                                                             \
	do {                                                                    \
		if (!(cond)) {                                                  \
			printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
			failures++;                                             \
		}                                                               \
	} while (0)

static uint8_t alpha_at(const struct ss_image *img, uint32_t x, uint32_t y)
{
	return img->rgba[((size_t)y * img->w + x) * 4 + 3];
}

static void test_toast_card_opacity(void)
{
	static const uint32_t sizes[][2] = {{384, 120}, {360, 96}, {200, 64}, {64, 40}, {33, 33}};
	for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
		struct ss_image img;
		CHECK(ss_gen_toast_card(sizes[i][0], sizes[i][1], &img));
		/* Everything at least max_corner_inset away from the edges
		 * must be fully opaque. */
		CHECK(ss_image_opaque_inside(&img, ss_plate_max_corner_inset()));
		/* Center pixel is opaque. */
		CHECK(alpha_at(&img, img.w / 2, img.h / 2) == 255);
		ss_image_free(&img);
	}
}

static void test_toast_card_corners_transparent(void)
{
	/* The literal corner pixel lies outside the rounded shape. */
	struct ss_image img;
	CHECK(ss_gen_toast_card(360, 96, &img));
	CHECK(alpha_at(&img, 0, 0) == 0);
	CHECK(alpha_at(&img, img.w - 1, 0) == 0);
	CHECK(alpha_at(&img, 0, img.h - 1) == 0);
	CHECK(alpha_at(&img, img.w - 1, img.h - 1) == 0);
	/* Edge midpoints are inside the shape -> opaque. */
	CHECK(alpha_at(&img, img.w / 2, 0) == 255);
	CHECK(alpha_at(&img, 0, img.h / 2) == 255);
	ss_image_free(&img);
}

static void test_privacy_plate_opacity(void)
{
	static const uint32_t sizes[][2] = {{640, 480}, {320, 220}, {128, 48}, {48, 128}, {32, 32}};
	for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
		struct ss_image img;
		CHECK(ss_gen_privacy_plate(sizes[i][0], sizes[i][1], &img));
		CHECK(ss_image_opaque_inside(&img, ss_plate_max_corner_inset()));
		CHECK(alpha_at(&img, img.w / 2, img.h / 2) == 255);
		ss_image_free(&img);
	}
}

static void test_status_banner_fully_opaque(void)
{
	/* The status chip has square corners: every single pixel must be
	 * opaque, inset 0. */
	struct ss_image img;
	CHECK(ss_gen_status_banner(&img));
	CHECK(img.w > 200 && img.h >= 40);
	CHECK(ss_image_opaque_inside(&img, 0));
	ss_image_free(&img);
}

static void test_no_rgb_bleed_means_text_present(void)
{
	/* Weak sanity check that the label actually rendered: the card
	 * must contain some near-white pixels (text color 235) besides
	 * the dark background. */
	struct ss_image img;
	CHECK(ss_gen_toast_card(384, 120, &img));
	int bright = 0;
	for (uint32_t y = 0; y < img.h; y++)
		for (uint32_t x = 0; x < img.w; x++) {
			const uint8_t *p = &img.rgba[((size_t)y * img.w + x) * 4];
			if (p[0] > 220 && p[1] > 220 && p[2] > 220)
				bright++;
		}
	CHECK(bright > 100);
	ss_image_free(&img);
}

static void test_degenerate_sizes_do_not_crash(void)
{
	struct ss_image img;
	CHECK(ss_gen_toast_card(8, 8, &img));
	ss_image_free(&img);
	CHECK(ss_gen_privacy_plate(8, 8, &img));
	ss_image_free(&img);
	CHECK(!ss_gen_toast_card(0, 100, &img));
	CHECK(!ss_gen_privacy_plate(100, 0, &img));
	CHECK(!ss_gen_toast_card(50000, 50000, &img));
}

static void test_padding_exceeds_corner_inset(void)
{
	/* The filter draws plates on rects padded by >= 8px (default 12).
	 * The transparency at rounded corners must stay strictly inside
	 * that padding margin, or content could peek out. */
	CHECK(ss_plate_max_corner_inset() < 8);
}

int main(void)
{
	test_toast_card_opacity();
	test_toast_card_corners_transparent();
	test_privacy_plate_opacity();
	test_status_banner_fully_opaque();
	test_no_rgb_bleed_means_text_present();
	test_degenerate_sizes_do_not_crash();
	test_padding_exceeds_corner_inset();

	if (failures) {
		printf("plate-gen-tests: %d FAILURE(S)\n", failures);
		return 1;
	}
	printf("plate-gen-tests: all passed\n");
	return 0;
}
