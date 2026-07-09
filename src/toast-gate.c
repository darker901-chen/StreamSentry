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

#include "toast-gate.h"

#include <math.h>

/* ---- constants (SPEC 2.2) -------------------------------------------
 *
 * PROVISIONAL, owner ruling 2026-07-09 (reports/M6-toast-probe.txt):
 * toast banners are currently system-suppressed on the dev machine
 * (delivered to the Notification Center only), so these bounds are
 * derived from documented Windows toast metrics instead of a live
 * capture, chosen GENEROUS in the over-mask direction. Empirical
 * calibration + re-verification of the process/class signature on
 * Win11 26200.8655 happen at the owner's acceptance pass.
 *
 * Basis: a Windows toast banner is a fixed-width surface of 396 DIP
 * (ToastGeneric), content-dependent height (~79 DIP collapsed to ~400
 * DIP expanded), anchored to the right work-area edge on LTR systems
 * (inset ~16 DIP), sliding in from the right, stacking vertically from
 * the anchor corner. Physical px = DIP * scale, scale 100-250%.
 *
 * Derivations (physical px):
 *   width  396 DIP at 50%..250%  -> 198..990  -> [200, 1000]
 *   height 79 DIP collapsed @100% -> 79; lower bound 60 keeps margin
 *          for trimmed banners while excluding one-line tooltips
 *          (~30-50 px); upper 1200 covers expanded/grouped banners at
 *          high scale
 *   edge   observed ~16 DIP inset -> 40 px at 250%; band 160 px gives
 *          4x margin (and absorbs taskbar-side offsets)
 *   caps   width <= 60% / height <= 90% of the monitor kill full-width
 *          and full-screen shell surfaces at any DPI
 *
 * Over-mask-safety argument (SPEC 2.2 requires it here): this gate only
 * ever REMOVES masking from signature-matched windows, so a too-narrow
 * rule would expose real toasts (unacceptable), while a too-wide rule
 * merely re-admits some flyover plates (cosmetic). Hence: bounds sit
 * far outside every documented variant; the band spans the FULL right
 * edge (top-right and bottom-right anchor variants both covered); the
 * height band is content-agnostic; and any degenerate input classifies
 * as toast. The v0.1 acceptance row "real toast masked before readable"
 * remains binding at acceptance time.
 */
#define SS_TOAST_EDGE_BAND_PX 160.0
#define SS_TOAST_W_MIN 200.0
#define SS_TOAST_W_MAX 1000.0
#define SS_TOAST_H_MIN 60.0
#define SS_TOAST_H_MAX 1200.0
#define SS_TOAST_W_MON_FRAC 0.60
#define SS_TOAST_H_MON_FRAC 0.90

static bool rect_sane(const struct ss_rect *r)
{
	return isfinite(r->x) && isfinite(r->y) && isfinite(r->w) && isfinite(r->h);
}

bool ss_toast_geom_plausible(const struct ss_rect *mon, const struct ss_rect *win)
{
	/* Uncertainty -> mask (iron rule 1 direction). */
	if (!mon || !win || !rect_sane(mon) || !rect_sane(win))
		return true;
	if (mon->w <= 0.0 || mon->h <= 0.0)
		return true;

	/* A window the enum pass reported with degenerate size would have
	 * been dropped there; treat it as implausible only when we can
	 * actually reason about it. */
	if (win->w <= 0.0 || win->h <= 0.0)
		return true;

	/* Size envelope (absolute + relative to this monitor). */
	if (win->w < SS_TOAST_W_MIN || win->w > SS_TOAST_W_MAX)
		return false;
	if (win->h < SS_TOAST_H_MIN || win->h > SS_TOAST_H_MAX)
		return false;
	if (win->w > mon->w * SS_TOAST_W_MON_FRAC)
		return false;
	if (win->h > mon->h * SS_TOAST_H_MON_FRAC)
		return false;

	/* Must overlap this monitor at all (multi-monitor: the gate is
	 * evaluated per monitor and passes if plausible on any). */
	const double mon_r = mon->x + mon->w;
	const double mon_b = mon->y + mon->h;
	const double win_r = win->x + win->w;
	const double win_b = win->y + win->h;
	if (win_r <= mon->x || win->x >= mon_r)
		return false;
	if (win_b <= mon->y || win->y >= mon_b)
		return false;

	/* Right-edge spawn band, full monitor height (covers top-right and
	 * bottom-right anchors). The slide-in animation can place the
	 * window's right edge BEYOND the monitor edge, so the band is
	 * one-sided: right edge must reach at least (mon right - band). */
	if (win_r < mon_r - SS_TOAST_EDGE_BAND_PX)
		return false;

	return true;
}

bool ss_toast_geom_plausible_any(const struct ss_rect *mons, size_t num_mons, const struct ss_rect *win)
{
	/* No monitor data -> cannot evaluate -> mask. */
	if (!mons || num_mons == 0)
		return true;
	for (size_t i = 0; i < num_mons; i++) {
		if (ss_toast_geom_plausible(&mons[i], win))
			return true;
	}
	return false;
}
