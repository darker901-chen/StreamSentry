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

#include <limits.h>
#include <string.h>
#include <wchar.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dwmapi.h>

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

static bool is_monitor_capture(const char *id)
{
	return id && strcmp(id, "monitor_capture") == 0;
}

static bool is_window_capture(const char *id)
{
	return id && strcmp(id, "window_capture") == 0;
}

static bool rect_nonempty(const RECT *rc)
{
	return rc && rc->right > rc->left && rc->bottom > rc->top;
}

static bool rect_equal(const RECT *a, const RECT *b)
{
	return a->left == b->left && a->top == b->top && a->right == b->right && a->bottom == b->bottom;
}

static bool rect_size_matches(const RECT *rc, uint32_t w, uint32_t h)
{
	return rect_nonempty(rc) && (uint32_t)(rc->right - rc->left) == w &&
	       (uint32_t)(rc->bottom - rc->top) == h;
}

static bool wide_to_utf8(const wchar_t *src, char *dst, size_t dst_size)
{
	if (!src || !dst || dst_size == 0 || dst_size > INT_MAX)
		return false;
	return WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, src, -1, dst, (int)dst_size, NULL, NULL) > 0;
}

static bool get_window_title_utf8(HWND hwnd, char *out, size_t out_size)
{
	wchar_t title[2048];
	int expected = GetWindowTextLengthW(hwnd);
	if (expected < 0 || expected >= (int)_countof(title))
		return false;
	if (expected == 0) {
		title[0] = L'\0';
	} else {
		int copied = GetWindowTextW(hwnd, title, (int)_countof(title));
		if (copied <= 0 || copied != expected)
			return false; /* title changed while being read -> do not guess */
	}
	return wide_to_utf8(title, out, out_size);
}

static bool get_window_class_utf8(HWND hwnd, char *out, size_t out_size)
{
	wchar_t class_name[512];
	int copied = GetClassNameW(hwnd, class_name, (int)_countof(class_name));
	if (copied <= 0 || copied >= (int)_countof(class_name) - 1)
		return false;
	return wide_to_utf8(class_name, out, out_size);
}

static bool get_window_executable_utf8(HWND hwnd, char *out, size_t out_size)
{
	DWORD pid = 0;
	if (!GetWindowThreadProcessId(hwnd, &pid) || pid == 0)
		return false;

	HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!process)
		return false;

	wchar_t path[32768];
	DWORD path_len = (DWORD)_countof(path);
	BOOL ok = QueryFullProcessImageNameW(process, 0, path, &path_len);
	CloseHandle(process);
	if (!ok || path_len == 0 || path_len >= _countof(path))
		return false;
	path[path_len] = L'\0';

	const wchar_t *base = wcsrchr(path, L'\\');
	base = base ? base + 1 : path;
	return wide_to_utf8(base, out, out_size);
}

struct window_match_ctx {
	const char *title;
	const char *class_name;
	const char *executable;
	HWND match;
	int matches;
};

static BOOL CALLBACK window_match_proc(HWND hwnd, LPARAM lp)
{
	struct window_match_ctx *ctx = (struct window_match_ctx *)lp;
	char title[8192];
	char class_name[2048];
	char executable[2048];

	/* Title/class are cheap and usually reject every unrelated window. Only
	 * open the owning process after those two exact fields match. */
	if (!get_window_title_utf8(hwnd, title, sizeof(title)) ||
	    !get_window_class_utf8(hwnd, class_name, sizeof(class_name)))
		return TRUE;
	if (strcmp(title, ctx->title) != 0 || strcmp(class_name, ctx->class_name) != 0)
		return TRUE;
	if (!get_window_executable_utf8(hwnd, executable, sizeof(executable)) ||
	    _stricmp(executable, ctx->executable) != 0)
		return TRUE;

	ctx->matches++;
	ctx->match = (ctx->matches == 1) ? hwnd : NULL;
	return TRUE;
}

/* OBS's Windows Window Capture source publishes the identity of the HWND it is
 * actually hooked to through its get_hooked procedure. The procedure does not
 * expose the HWND itself, so resolve it only when the reported title/class/exe
 * tuple maps to exactly one current top-level window. Duplicate identities are
 * deliberately rejected: choosing either one would violate SPEC 2.7. */
static bool find_unique_hooked_window(obs_source_t *target, HWND *out_hwnd)
{
	proc_handler_t *handler = obs_source_get_proc_handler(target);
	if (!handler)
		return false;

	calldata_t params = {0};
	if (!proc_handler_call(handler, "get_hooked", &params)) {
		calldata_free(&params);
		return false;
	}

	bool hooked = calldata_bool(&params, "hooked");
	const char *title = calldata_string(&params, "title");
	const char *class_name = calldata_string(&params, "class");
	const char *executable = calldata_string(&params, "executable");
	if (!hooked || !title || !class_name || !executable || !*class_name || !*executable) {
		calldata_free(&params);
		return false;
	}

	struct window_match_ctx ctx = {
		.title = title,
		.class_name = class_name,
		.executable = executable,
		.match = NULL,
		.matches = 0,
	};
	BOOL enum_ok = EnumWindows(window_match_proc, (LPARAM)&ctx);
	calldata_free(&params);
	if (!enum_ok || ctx.matches != 1 || !ctx.match || !IsWindow(ctx.match))
		return false;

	*out_hwnd = ctx.match;
	return true;
}

static bool get_client_screen_rect(HWND hwnd, RECT *out)
{
	RECT client;
	if (!GetClientRect(hwnd, &client) || !rect_nonempty(&client))
		return false;

	POINT top_left = {client.left, client.top};
	POINT bottom_right = {client.right, client.bottom};
	if (!ClientToScreen(hwnd, &top_left) || !ClientToScreen(hwnd, &bottom_right))
		return false;

	out->left = top_left.x;
	out->top = top_left.y;
	out->right = bottom_right.x;
	out->bottom = bottom_right.y;
	return rect_nonempty(out);
}

static void add_unique_rect(RECT *rects, int *count, int capacity, const RECT *candidate)
{
	if (!rect_nonempty(candidate) || *count >= capacity)
		return;
	for (int i = 0; i < *count; i++) {
		if (rect_equal(&rects[i], candidate))
			return;
	}
	rects[(*count)++] = *candidate;
}

static bool resolve_window_capture(obs_source_t *target, uint32_t base_w, uint32_t base_h,
				   struct ss_capture_geom *out)
{
	HWND hwnd = NULL;
	if (!find_unique_hooked_window(target, &hwnd))
		return false;

	/* OBS can produce either the client area (BitBlt and WGC client-area
	 * mode) or a full-window frame (WGC). Do not duplicate OBS's private
	 * method-selection logic. Instead, gather the Windows-authoritative
	 * candidate rectangles and accept only one unique rectangle whose exact
	 * physical-pixel size equals the source's base size. */
	RECT candidates[3];
	int candidate_count = 0;
	RECT rc;
	if (get_client_screen_rect(hwnd, &rc))
		add_unique_rect(candidates, &candidate_count, 3, &rc);
	if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rc, (DWORD)sizeof(rc))))
		add_unique_rect(candidates, &candidate_count, 3, &rc);
	if (GetWindowRect(hwnd, &rc))
		add_unique_rect(candidates, &candidate_count, 3, &rc);

	RECT matches[3];
	int match_count = 0;
	for (int i = 0; i < candidate_count; i++) {
		if (rect_size_matches(&candidates[i], base_w, base_h))
			add_unique_rect(matches, &match_count, 3, &candidates[i]);
	}
	if (match_count != 1)
		return false;

	RECT selected = matches[0];
	out->screen_region.x = (double)selected.left;
	out->screen_region.y = (double)selected.top;
	out->screen_region.w = (double)(selected.right - selected.left);
	out->screen_region.h = (double)(selected.bottom - selected.top);
	out->src_w = (double)base_w;
	out->src_h = (double)base_h;
	return true;
}

static bool resolve_monitor_capture(uint32_t base_w, uint32_t base_h, struct ss_capture_geom *out)
{
	struct mon_list list;
	list.count = 0;
	EnumDisplayMonitors(NULL, NULL, mon_proc, (LPARAM)&list);
	if (list.count == 0)
		return false;

	/* Identify the captured monitor ONLY when it is unambiguous: exactly
	 * one monitor whose native pixel size equals the source base size.
	 * OBS's monitor index/device path is not portably mappable to the
	 * EnumDisplayMonitors ordering. */
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
	out->screen_region.x = (double)rc.left;
	out->screen_region.y = (double)rc.top;
	out->screen_region.w = (double)(rc.right - rc.left);
	out->screen_region.h = (double)(rc.bottom - rc.top);
	out->src_w = (double)base_w;
	out->src_h = (double)base_h;
	return true;
}

bool ss_resolve_capture_geom(obs_source_t *target, struct ss_capture_geom *out)
{
	if (!target || !out)
		return false;

	uint32_t base_w = obs_source_get_base_width(target);
	uint32_t base_h = obs_source_get_base_height(target);
	if (base_w == 0 || base_h == 0)
		return false;

	const char *id = obs_source_get_id(target);
	if (is_monitor_capture(id))
		return resolve_monitor_capture(base_w, base_h, out);
	if (is_window_capture(id))
		return resolve_window_capture(target, base_w, base_h, out);
	return false; /* unsupported source type -> caller renders degraded */
}
