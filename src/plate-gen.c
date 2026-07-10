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

#include "plate-gen.h"

#include <stdlib.h>
#include <string.h>

/* ---- tiny original 5x7 bitmap font -------------------------------- */
/* Only the glyphs the three fixed labels need. Bit 0x10 = leftmost
 * column, bit 0x01 = rightmost. Unknown characters render as a solid
 * block (over-mask friendly). Glyph shapes drawn by hand for this
 * project. */

struct ss_glyph {
	char ch;
	uint8_t rows[7];
};

static const struct ss_glyph FONT[] = {
	{'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
	{'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
	{'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
	{'a', {0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F}},
	{'b', {0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x1E}},
	{'c', {0x00, 0x00, 0x0F, 0x10, 0x10, 0x10, 0x0F}},
	{'d', {0x01, 0x01, 0x0F, 0x11, 0x11, 0x11, 0x0F}},
	{'e', {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E}},
	{'f', {0x06, 0x09, 0x08, 0x1E, 0x08, 0x08, 0x08}},
	{'g', {0x00, 0x0F, 0x11, 0x11, 0x0F, 0x01, 0x0E}},
	{'h', {0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x11}},
	{'i', {0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E}},
	{'k', {0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12}},
	{'l', {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
	{'n', {0x00, 0x00, 0x1E, 0x11, 0x11, 0x11, 0x11}},
	{'o', {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E}},
	{'p', {0x00, 0x00, 0x1E, 0x11, 0x1E, 0x10, 0x10}},
	{'r', {0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10}},
	{'s', {0x00, 0x00, 0x0F, 0x10, 0x0E, 0x01, 0x1E}},
	{'t', {0x08, 0x08, 0x1E, 0x08, 0x08, 0x09, 0x06}},
	{'u', {0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D}},
	{'v', {0x00, 0x00, 0x11, 0x11, 0x11, 0x0A, 0x04}},
	{'y', {0x00, 0x00, 0x11, 0x11, 0x0F, 0x01, 0x0E}},
	{':', {0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00}},
	{'-', {0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00}},
	{' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
};

static const uint8_t GLYPH_UNKNOWN[7] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};

static const uint8_t *find_glyph(char ch)
{
	for (size_t i = 0; i < sizeof(FONT) / sizeof(FONT[0]); i++) {
		if (FONT[i].ch == ch)
			return FONT[i].rows;
	}
	return GLYPH_UNKNOWN;
}

/* ---- pixel helpers ------------------------------------------------- */

static void put_px(struct ss_image *img, int64_t x, int64_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	if (x < 0 || y < 0 || x >= (int64_t)img->w || y >= (int64_t)img->h)
		return;
	uint8_t *p = img->rgba + ((size_t)y * img->w + (size_t)x) * 4;
	p[0] = r;
	p[1] = g;
	p[2] = b;
	p[3] = a;
}

static void fill_rect(struct ss_image *img, int64_t x, int64_t y, int64_t w, int64_t h, uint8_t r, uint8_t g,
		      uint8_t b, uint8_t a)
{
	for (int64_t yy = y; yy < y + h; yy++)
		for (int64_t xx = x; xx < x + w; xx++)
			put_px(img, xx, yy, r, g, b, a);
}

static void fill_circle(struct ss_image *img, int64_t cx, int64_t cy, int64_t rad, uint8_t r, uint8_t g, uint8_t b,
			uint8_t a)
{
	for (int64_t yy = cy - rad; yy <= cy + rad; yy++) {
		for (int64_t xx = cx - rad; xx <= cx + rad; xx++) {
			int64_t dx = xx - cx, dy = yy - cy;
			if (dx * dx + dy * dy <= rad * rad)
				put_px(img, xx, yy, r, g, b, a);
		}
	}
}

/* Rounded-rect fill covering the whole image with corner radius rad.
 * Pixels outside the rounded corners stay untouched (transparent). */
static void fill_rounded(struct ss_image *img, int64_t rad, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	const int64_t w = (int64_t)img->w, h = (int64_t)img->h;
	for (int64_t y = 0; y < h; y++) {
		for (int64_t x = 0; x < w; x++) {
			int64_t dx = 0, dy = 0;
			if (x < rad && y < rad) {
				dx = rad - 1 - x;
				dy = rad - 1 - y;
			} else if (x >= w - rad && y < rad) {
				dx = x - (w - rad);
				dy = rad - 1 - y;
			} else if (x < rad && y >= h - rad) {
				dx = rad - 1 - x;
				dy = y - (h - rad);
			} else if (x >= w - rad && y >= h - rad) {
				dx = x - (w - rad);
				dy = y - (h - rad);
			}
			if (dx * dx + dy * dy <= (rad - 1) * (rad - 1) || (dx == 0 && dy == 0))
				put_px(img, x, y, r, g, b, a);
			else if (dx * dx + dy * dy <= rad * rad)
				put_px(img, x, y, r, g, b, a);
		}
	}
}

static uint32_t text_width(uint32_t scale, const char *s)
{
	size_t n = strlen(s);
	if (n == 0)
		return 0;
	/* 5 columns + 1 space column per char, minus trailing space */
	return (uint32_t)(n * 6 - 1) * scale;
}

static void draw_text(struct ss_image *img, int64_t x, int64_t y, uint32_t scale, uint8_t r, uint8_t g, uint8_t b,
		      const char *s)
{
	int64_t pen = x;
	for (const char *c = s; *c; c++) {
		const uint8_t *rows = find_glyph(*c);
		for (int row = 0; row < 7; row++) {
			for (int col = 0; col < 5; col++) {
				if (rows[row] & (0x10 >> col)) {
					fill_rect(img, pen + (int64_t)col * scale, y + (int64_t)row * scale,
						  scale, scale, r, g, b, 255);
				}
			}
		}
		pen += (int64_t)6 * scale;
	}
}

static bool alloc_image(uint32_t w, uint32_t h, struct ss_image *out)
{
	if (w == 0 || h == 0 || w > 16384 || h > 16384)
		return false;
	out->rgba = calloc((size_t)w * h, 4); /* all transparent */
	if (!out->rgba)
		return false;
	out->w = w;
	out->h = h;
	return true;
}

/* ---- glyph art ------------------------------------------------------ */

static void draw_bell(struct ss_image *img, int64_t cx, int64_t cy, int64_t size)
{
	const uint8_t r = 214, g = 214, b = 224;
	int64_t rad = size / 2;
	if (rad < 3)
		return;
	/* dome */
	fill_circle(img, cx, cy - rad / 4, (rad * 3) / 4, r, g, b, 255);
	/* body below dome down to the rim */
	fill_rect(img, cx - (rad * 3) / 4, cy - rad / 4, (rad * 3) / 2, (rad * 3) / 4, r, g, b, 255);
	/* rim bar */
	fill_rect(img, cx - rad, cy + rad / 2 - 1, rad * 2, (rad / 3 > 2) ? rad / 3 : 2, r, g, b, 255);
	/* clapper */
	fill_circle(img, cx, cy + rad - rad / 8, rad / 4 > 1 ? rad / 4 : 1, r, g, b, 255);
}

static void draw_lock(struct ss_image *img, int64_t cx, int64_t cy, int64_t size, uint8_t bg_r, uint8_t bg_g,
		      uint8_t bg_b)
{
	const uint8_t r = 208, g = 208, b = 218;
	int64_t half = size / 2;
	if (half < 4)
		return;
	int64_t body_w = size, body_h = (size * 3) / 4;
	int64_t body_x = cx - half, body_y = cy - body_h / 4;
	int64_t sh_rad = (size * 3) / 8; /* shackle outer radius */
	int64_t sh_cy = body_y;

	/* shackle ring (upper half visible above the body) */
	fill_circle(img, cx, sh_cy, sh_rad, r, g, b, 255);
	fill_circle(img, cx, sh_cy, sh_rad - (size / 8 > 2 ? size / 8 : 2), bg_r, bg_g, bg_b, 255);
	/* body (covers lower half of the ring) */
	fill_rect(img, body_x, body_y, body_w, body_h, r, g, b, 255);
	/* keyhole */
	fill_circle(img, cx, body_y + body_h / 2, size / 10 > 1 ? size / 10 : 1, bg_r, bg_g, bg_b, 255);
}

/* ---- public generators ---------------------------------------------- */

#define CARD_RADIUS 12
#define PLATE_RADIUS 6

bool ss_gen_toast_card(uint32_t w, uint32_t h, struct ss_image *out)
{
	if (!out || !alloc_image(w, h, out))
		return false;

	int64_t rad = CARD_RADIUS;
	if (rad > (int64_t)w / 4)
		rad = w / 4;
	if (rad > (int64_t)h / 4)
		rad = h / 4;

	/* border ring then card fill */
	fill_rounded(out, rad, 58, 58, 70, 255);
	{
		/* inner fill: cheap 2px inset border by overdrawing the
		 * interior rect (corners keep the border color) */
		fill_rect(out, 2, 2, (int64_t)w - 4, (int64_t)h - 4, 32, 32, 40, 255);
	}

	const uint32_t scale = 2;
	const char *label = "Notification hidden";
	uint32_t tw = text_width(scale, label);
	int64_t bell_size = (int64_t)h / 2;
	if (bell_size > 28)
		bell_size = 28;

	bool room_for_bell = ((int64_t)h >= 30) && ((int64_t)w >= bell_size + 24);
	bool room_for_text = room_for_bell && ((int64_t)w >= bell_size + 36 + (int64_t)tw) && ((int64_t)h >= 26);

	if (room_for_bell) {
		int64_t bx = 16 + bell_size / 2;
		if (!room_for_text)
			bx = (int64_t)w / 2; /* center the bell alone */
		draw_bell(out, bx, (int64_t)h / 2, bell_size);
	}
	if (room_for_text) {
		int64_t tx = 16 + bell_size + 12;
		int64_t ty = ((int64_t)h - 7 * (int64_t)scale) / 2;
		draw_text(out, tx, ty, scale, 235, 235, 240, label);
	}
	return true;
}

bool ss_gen_privacy_plate(uint32_t w, uint32_t h, struct ss_image *out)
{
	if (!out || !alloc_image(w, h, out))
		return false;

	int64_t rad = PLATE_RADIUS;
	if (rad > (int64_t)w / 4)
		rad = w / 4;
	if (rad > (int64_t)h / 4)
		rad = h / 4;

	fill_rounded(out, rad, 44, 44, 54, 255);
	fill_rect(out, 2, 2, (int64_t)w - 4, (int64_t)h - 4, 18, 18, 24, 255);

	const uint32_t scale = 2;
	const char *label = "Hidden";
	uint32_t tw = text_width(scale, label);
	int64_t lock_size = (int64_t)(w < h ? w : h) / 4;
	if (lock_size > 32)
		lock_size = 32;

	bool stacked = (int64_t)h >= lock_size + 7 * (int64_t)scale + 28;
	bool room_lock = lock_size >= 12;
	bool room_text = (int64_t)w >= (int64_t)tw + 12 && (int64_t)h >= 7 * (int64_t)scale + 8;

	if (room_lock && stacked) {
		int64_t cy = (int64_t)h / 2 - (room_text ? (7 * (int64_t)scale + 8) / 2 : 0);
		draw_lock(out, (int64_t)w / 2, cy, lock_size, 18, 18, 24);
		if (room_text)
			draw_text(out, ((int64_t)w - (int64_t)tw) / 2, cy + lock_size / 2 + 14, scale, 225, 225,
				  232, label);
	} else if (room_text) {
		draw_text(out, ((int64_t)w - (int64_t)tw) / 2, ((int64_t)h - 7 * (int64_t)scale) / 2, scale, 225,
			  225, 232, label);
	} else if (room_lock) {
		draw_lock(out, (int64_t)w / 2, (int64_t)h / 2, lock_size, 18, 18, 24);
	}
	return true;
}

bool ss_gen_status_banner(struct ss_image *out)
{
	const char *label = "StreamSentry: protection degraded - see log";
	const uint32_t scale = 2;
	uint32_t tw = text_width(scale, label);
	uint32_t w = tw + 48;
	uint32_t h = 7 * scale + 32;

	if (!out || !alloc_image(w, h, out))
		return false;

	/* opaque everywhere: square corners on purpose */
	fill_rect(out, 0, 0, w, h, 96, 14, 14, 255);
	fill_rect(out, 2, 2, (int64_t)w - 4, (int64_t)h - 4, 128, 20, 20, 255);
	draw_text(out, 24, ((int64_t)h - 7 * (int64_t)scale) / 2, scale, 255, 244, 244, label);
	return true;
}

void ss_image_free(struct ss_image *img)
{
	if (img && img->rgba) {
		free(img->rgba);
		img->rgba = NULL;
		img->w = img->h = 0;
	}
}

bool ss_image_opaque_inside(const struct ss_image *img, uint32_t inset)
{
	if (!img || !img->rgba)
		return false;
	if (img->w <= inset * 2 || img->h <= inset * 2)
		return false;
	for (uint32_t y = inset; y < img->h - inset; y++) {
		for (uint32_t x = inset; x < img->w - inset; x++) {
			if (img->rgba[((size_t)y * img->w + x) * 4 + 3] != 255)
				return false;
		}
	}
	return true;
}

uint32_t ss_plate_max_corner_inset(void)
{
	/* Transparent pixels exist only outside the corner quarter-circles.
	 * For radius r the deepest such pixel sits r*(1 - 1/sqrt(2)) from
	 * the corner along the diagonal; for r = 12 that is < 4.6px. 5 is
	 * a safe conservative bound. Filter-side padding (>= 8, default 12)
	 * must stay above this. */
	return 5;
}
