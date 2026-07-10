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

/* Pure CPU generator for the mask visuals (RGBA8888 buffers). No OBS or
 * Windows dependencies so opacity — the security property of iron rule 3
 * — is enforceable by unit tests, not just by eyeballing.
 *
 * Iron rule 3: masking must be opaque. Inside the plate shape every
 * pixel has alpha 255. The only transparent pixels are the sliver
 * outside the rounded corners; callers must draw plates padded 8-16px
 * beyond the detected rect (coord-map pad), which keeps the sensitive
 * area strictly inside the opaque region (corner inset < pad).
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ss_image {
	uint32_t w;
	uint32_t h;
	uint8_t *rgba; /* w * h * 4 bytes, heap-allocated */
};

/* Notification-shaped placeholder card: rounded rect, bell glyph,
 * "Notification hidden" label (label omitted when too small to fit). */
bool ss_gen_toast_card(uint32_t w, uint32_t h, struct ss_image *out);

/* Privacy plate: dark fill, lock glyph, "Hidden" label. */
bool ss_gen_privacy_plate(uint32_t w, uint32_t h, struct ss_image *out);

/* Failure status chip: "StreamSentry: protection degraded - see log"
 * on an opaque background, sized to the text. Drawn small in a corner
 * of the normally-rendered output whenever protection cannot be fully
 * verified (owner ruling 2026-07-09 — no blackout, never silent). */
bool ss_gen_status_banner(struct ss_image *out);

void ss_image_free(struct ss_image *img);

/* Test helpers (also used by asserts in debug builds). */

/* True if every pixel with x/y at least `inset` away from every edge is
 * fully opaque (alpha == 255). */
bool ss_image_opaque_inside(const struct ss_image *img, uint32_t inset);

/* Corner inset: max distance from a plate corner at which transparent
 * pixels may exist. Callers' pad must exceed this. */
uint32_t ss_plate_max_corner_inset(void);

#ifdef __cplusplus
}
#endif
