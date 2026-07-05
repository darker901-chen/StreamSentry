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

#include "coord-map.h"

#include <math.h>

static bool rect_finite(const struct ss_rect *r)
{
	return isfinite(r->x) && isfinite(r->y) && isfinite(r->w) && isfinite(r->h);
}

enum ss_map_result ss_map_screen_rect(const struct ss_capture_geom *geom, const struct ss_rect *screen_rect,
				      double pad_px, struct ss_rect *out)
{
	if (!geom || !screen_rect || !out)
		return SS_MAP_INVALID;
	if (!rect_finite(&geom->screen_region) || !rect_finite(screen_rect))
		return SS_MAP_INVALID;
	if (!isfinite(geom->src_w) || !isfinite(geom->src_h) || !isfinite(pad_px))
		return SS_MAP_INVALID;
	if (geom->screen_region.w <= 0.0 || geom->screen_region.h <= 0.0)
		return SS_MAP_INVALID;
	if (geom->src_w <= 0.0 || geom->src_h <= 0.0)
		return SS_MAP_INVALID;
	if (screen_rect->w < 0.0 || screen_rect->h < 0.0 || pad_px < 0.0)
		return SS_MAP_INVALID;

	/* Intersect with the captured region in screen space. */
	const double rx0 = geom->screen_region.x;
	const double ry0 = geom->screen_region.y;
	const double rx1 = rx0 + geom->screen_region.w;
	const double ry1 = ry0 + geom->screen_region.h;

	const double ix0 = fmax(screen_rect->x, rx0);
	const double iy0 = fmax(screen_rect->y, ry0);
	const double ix1 = fmin(screen_rect->x + screen_rect->w, rx1);
	const double iy1 = fmin(screen_rect->y + screen_rect->h, ry1);

	if (ix0 >= ix1 || iy0 >= iy1)
		return SS_MAP_NOT_VISIBLE;

	/* Scale into source pixel space (handles mixed-DPI monitors and
	 * scaled captures: each geom carries its own physical region and
	 * source size, so the ratio below is per-capture). */
	const double sx = geom->src_w / geom->screen_region.w;
	const double sy = geom->src_h / geom->screen_region.h;

	double ox0 = (ix0 - rx0) * sx - pad_px;
	double oy0 = (iy0 - ry0) * sy - pad_px;
	double ox1 = (ix1 - rx0) * sx + pad_px;
	double oy1 = (iy1 - ry0) * sy + pad_px;

	/* Clamp to source bounds; padding may not push mask off-source. */
	ox0 = fmax(ox0, 0.0);
	oy0 = fmax(oy0, 0.0);
	ox1 = fmin(ox1, geom->src_w);
	oy1 = fmin(oy1, geom->src_h);

	if (!(isfinite(ox0) && isfinite(oy0) && isfinite(ox1) && isfinite(oy1)))
		return SS_MAP_INVALID;
	if (ox0 >= ox1 || oy0 >= oy1)
		return SS_MAP_NOT_VISIBLE;

	out->x = ox0;
	out->y = oy0;
	out->w = ox1 - ox0;
	out->h = oy1 - oy0;
	return SS_MAP_OK;
}
