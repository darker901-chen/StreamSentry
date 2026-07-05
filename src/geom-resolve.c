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

#include "geom-resolve.h"

#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct mon_entry {
	RECT rc;
};

struct mon_list {
	struct mon_entry mons[16];
	int count;
};

static BOOL CALLBACK mon_proc(HMONITOR h, HDC dc, LPRECT rc, LPARAM lp)
{
	UNUSED_PARAMETER(dc);
	struct mon_list *list = (struct mon_list *)lp;
	if (list->count < 16) {
		MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		if (GetMonitorInfo(h, &mi)) {
			list->mons[list->count].rc = mi.rcMonitor;
			list->count++;
		}
	}
	UNUSED_PARAMETER(rc);
	return TRUE;
}

/* Is the display/monitor-capture source id? Covers the current dxgi/gdi
 * monitor capture ("monitor_capture") on Windows. Window/game capture
 * deliberately excluded — their region is not resolvable here. */
static bool is_monitor_capture(const char *id)
{
	return id && strcmp(id, "monitor_capture") == 0;
}

bool ss_resolve_capture_geom(obs_source_t *target, struct ss_capture_geom *out)
{
	if (!target || !out)
		return false;

	const char *id = obs_source_get_id(target);
	if (!is_monitor_capture(id))
		return false; /* unsupported source type -> caller fails closed */

	uint32_t base_w = obs_source_get_base_width(target);
	uint32_t base_h = obs_source_get_base_height(target);
	if (base_w == 0 || base_h == 0)
		return false;

	struct mon_list list;
	list.count = 0;
	EnumDisplayMonitors(NULL, NULL, mon_proc, (LPARAM)&list);
	if (list.count == 0)
		return false;

	/* Identify the captured monitor ONLY when it is unambiguous: exactly
	 * one monitor whose native pixel size equals the source base size.
	 *
	 * We deliberately do NOT trust the source's monitor index/id: OBS's
	 * "monitor" index (when present at all — modern builds store a
	 * "monitor_id" device path we cannot portably map to an
	 * EnumDisplayMonitors entry) is not guaranteed to match this
	 * enumeration order, and a same-resolution neighbour would pass a
	 * size check while giving the WRONG origin -> a mask placed off the
	 * sensitive region. Per iron rule 1 that under-mask is unacceptable,
	 * so an ambiguous match (two identical-resolution monitors, or none
	 * matching) resolves to failure and the caller fails closed.
	 * Limitation: dual identical-resolution monitors are not
	 * distinguishable here in v0.1 (documented). */
	int match = -1, matches = 0;
	for (int i = 0; i < list.count; i++) {
		long mw = list.mons[i].rc.right - list.mons[i].rc.left;
		long mh = list.mons[i].rc.bottom - list.mons[i].rc.top;
		if (mw == (long)base_w && mh == (long)base_h) {
			match = i;
			matches++;
		}
	}
	if (matches != 1)
		return false;

	RECT rc = list.mons[match].rc;
	long mw = rc.right - rc.left;
	long mh = rc.bottom - rc.top;

	out->screen_region.x = (double)rc.left;
	out->screen_region.y = (double)rc.top;
	out->screen_region.w = (double)mw;
	out->screen_region.h = (double)mh;
	out->src_w = (double)base_w;
	out->src_h = (double)base_h;
	return true;
}
