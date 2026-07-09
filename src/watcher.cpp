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

#include "watcher.h"
#include "shared-state.h"
#include "toast-gate.h"

#include <util/base.h> /* LOG_INFO / LOG_WARNING / LOG_ERROR enum */

/* Declared plainly (no dllimport) rather than via <plugin-support.h> /
 * <util/platform.h>: those decorate these symbols with dllimport, which
 * (a) clashes with plugin-support.h's own blogva declaration under C++
 * and (b) would break the standalone watcher-selftest, which links
 * neither obs.dll nor plugin-support and supplies its own definitions.
 * A plain declaration resolves via the obs import-lib thunk in the real
 * plugin and via the test stub in the self-test. */
extern "C" {
void obs_log(int log_level, const char *format, ...);
uint64_t os_gettime_ns(void);
}

/* NOTE: no WIN32_LEAN_AND_MEAN here. The UI Automation headers
 * (uiautomation.h -> UIAutomationCore.h) require the full OLE/COM stack
 * (ole2.h, oleauto.h) to define the `interface` keyword as `struct` and
 * the base provider interfaces consistently; excluding it triggers
 * C2371 "redefinition; different basic types" across the provider
 * interfaces. */
#define NOMINMAX /* keep windows.h's min/max macros from shadowing std::min */
#include <windows.h>
#include <objbase.h>
#include <oleauto.h>
#include <dwmapi.h>
#include <psapi.h>
#include <uiautomation.h>

#include <string>
#include <vector>
#include <algorithm>
#include <mutex>
#include <unordered_map>

/* ---- tunables ------------------------------------------------------- */

/* Enumeration cadence. SPEC: 100-200ms. */
static const DWORD WATCH_TICK_MS = 150;

/* Early-warning threshold for a slow tick (SPEC 2.1): half the render
 * side's 500ms fail-closed stale threshold. Always compiled, not just
 * perf builds — one LOG_WARNING per offending tick. */
static const uint64_t TICK_WARN_NS = 250000000ULL;

/* Toast signature for this Windows build family. Determined empirically
 * on Windows 11 build 26200 (25H2): a toast banner is a top-level window
 * owned by explorer.exe with class "Xaml_WindowedPopupClass". This
 * REPLACES SPEC.md's Win10-era example (ShellExperienceHost CoreWindow),
 * which does not host the toast on Win11. Signature = process AND class
 * (CLAUDE.md). Known over-match: explorer also uses this class for other
 * XAML flyouts (Start search, taskbar popups); per iron rule 1 we
 * over-mask those rather than risk missing a toast. Phantom 0x0 popups
 * are filtered by the visible/non-cloaked/non-empty gate below. */
static const wchar_t *TOAST_PROC = L"explorer.exe";
static const wchar_t *TOAST_CLASS = L"xaml_windowedpopupclass";

/* Built-in blocklist defaults (SPEC: password managers + system
 * credential dialogs only). Matched as case-insensitive substrings
 * against BOTH the process image name and the window title (see
 * reports/M2-DECISIONS.md for the CLAUDE.md-vs-SPEC reconciliation). */
static const wchar_t *DEFAULT_BLOCKLIST[] = {
	L"1password", L"keepass", L"bitwarden", L"dashlane", L"lastpass",
	L"consent.exe",          /* UAC elevation dialog host */
	L"credentialuibroker",   /* Windows Security credential prompt */
	L"logonui.exe",          /* logon / credential UI */
};

/* ---- module state --------------------------------------------------- */

namespace {

std::mutex g_mutex;             /* guards start/stop refcount + config */
int g_refcount = 0;
HANDLE g_thread = nullptr;
HANDLE g_stop_event = nullptr;
volatile LONG g_killed = 0;     /* fault-injection flag */

std::mutex g_cfg_mutex;         /* guards blocklist */
std::vector<std::wstring> g_blocklist;

std::mutex g_pw_mutex;          /* guards password-field rect */
bool g_pw_valid = false;
RECT g_pw_rect = {0, 0, 0, 0};

std::wstring to_lower(std::wstring s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return (wchar_t)towlower(c); });
	return s;
}

std::wstring proc_image_name(DWORD pid)
{
	std::wstring result;
	HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!h)
		return result;
	wchar_t buf[MAX_PATH];
	DWORD sz = MAX_PATH;
	if (QueryFullProcessImageNameW(h, 0, buf, &sz)) {
		std::wstring full(buf, sz);
		size_t slash = full.find_last_of(L"\\/");
		result = (slash == std::wstring::npos) ? full : full.substr(slash + 1);
	}
	CloseHandle(h);
	return to_lower(result);
}

/* PID -> image-name cache (M6, SPEC 2.1). v0.1 called OpenProcess +
 * QueryFullProcessImageNameW for EVERY visible window EVERY tick; under
 * streaming load that is the suspected cause of the rare >500ms ticks
 * that tripped fail-closed. With the cache, the OS is queried only for
 * PIDs not observed in the previous tick.
 *
 * Owned exclusively by the watcher thread (EnumWindows runs
 * synchronously on it) — no locking.
 *
 * Eviction: any PID not observed for one full tick is erased at the end
 * of that tick, so after any gap the name is re-queried fresh.
 *
 * Residual race, accepted and documented per SPEC 2.1 / the M5
 * spec-guardian note: if a process exits and Windows reuses its PID for
 * a NEW process whose window becomes visible within the SAME 150ms
 * tick-to-tick window (so the PID is never absent for a full tick), one
 * or more ticks can match against the stale name. In practice the gap
 * between process exit and a fresh process mapping a visible top-level
 * window exceeds one tick; title-substring matching (OR semantics,
 * ruling a434b18) is unaffected either way.
 *
 * Failed lookups (empty name, e.g. OpenProcess denied) are deliberately
 * NOT cached: v0.1 retried those every tick, and caching the failure
 * would silently drop proc-name matching for that window's lifetime.
 * The retry cost is bounded to the few windows whose query fails. */
struct PidCacheEntry {
	std::wstring name;
	bool seen_this_tick;
};
std::unordered_map<DWORD, PidCacheEntry> g_pid_cache;

#ifdef STREAMSENTRY_PERF_LOG
uint64_t g_cache_hits = 0, g_cache_misses = 0, g_cache_evictions = 0;
#endif

std::wstring cached_proc_image_name(DWORD pid)
{
	auto it = g_pid_cache.find(pid);
	if (it != g_pid_cache.end()) {
		it->second.seen_this_tick = true;
#ifdef STREAMSENTRY_PERF_LOG
		g_cache_hits++;
#endif
		return it->second.name;
	}
	std::wstring name = proc_image_name(pid);
#ifdef STREAMSENTRY_PERF_LOG
	g_cache_misses++;
#endif
	if (!name.empty()) {
		PidCacheEntry e;
		e.name = name;
		e.seen_this_tick = true;
		g_pid_cache.emplace(pid, e);
	}
	return name;
}

void pid_cache_begin_tick()
{
	for (auto &kv : g_pid_cache)
		kv.second.seen_this_tick = false;
}

void pid_cache_end_tick()
{
	for (auto it = g_pid_cache.begin(); it != g_pid_cache.end();) {
		if (!it->second.seen_this_tick) {
			it = g_pid_cache.erase(it);
#ifdef STREAMSENTRY_PERF_LOG
			g_cache_evictions++;
#endif
		} else {
			++it;
		}
	}
}

bool is_cloaked(HWND hwnd)
{
	int cloaked = 0;
	if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))))
		return cloaked != 0;
	return false;
}

/* Collected during a single EnumWindows pass. */
#define SS_MAX_MONITORS 16

struct EnumCtx {
	std::vector<ss_shared_rect> rects;
	std::vector<std::wstring> *blocklist;
	ss_rect mons[SS_MAX_MONITORS]; /* refreshed each tick for the toast gate */
	size_t num_mons;
	bool mons_complete; /* false = truncated or partially resolved */
};

BOOL CALLBACK mon_enum_proc(HMONITOR hmon, HDC, LPRECT, LPARAM lp)
{
	EnumCtx *ctx = reinterpret_cast<EnumCtx *>(lp);
	if (ctx->num_mons >= SS_MAX_MONITORS) {
		ctx->mons_complete = false; /* >16 monitors: list truncated */
		return FALSE;
	}
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	if (GetMonitorInfoW(hmon, &mi)) {
		ss_rect &r = ctx->mons[ctx->num_mons++];
		r.x = (double)mi.rcMonitor.left;
		r.y = (double)mi.rcMonitor.top;
		r.w = (double)(mi.rcMonitor.right - mi.rcMonitor.left);
		r.h = (double)(mi.rcMonitor.bottom - mi.rcMonitor.top);
	} else {
		ctx->mons_complete = false; /* a monitor we cannot place */
	}
	return TRUE;
}

BOOL CALLBACK enum_proc(HWND hwnd, LPARAM lp)
{
	EnumCtx *ctx = reinterpret_cast<EnumCtx *>(lp);
	if (ctx->rects.size() >= SS_MAX_RECTS)
		return FALSE;

	if (!IsWindowVisible(hwnd))
		return TRUE;
	if (is_cloaked(hwnd))
		return TRUE;

	RECT rc;
	if (!GetWindowRect(hwnd, &rc))
		return TRUE;
	long w = rc.right - rc.left, h = rc.bottom - rc.top;
	if (w <= 0 || h <= 0)
		return TRUE;

	wchar_t cls[256] = {0};
	GetClassNameW(hwnd, cls, 256);
	std::wstring cls_l = to_lower(cls);

	int len = GetWindowTextLengthW(hwnd);
	std::wstring title;
	if (len > 0) {
		title.resize(len + 1);
		int got = GetWindowTextW(hwnd, &title[0], len + 1);
		title.resize(got < 0 ? 0 : got);
	}
	std::wstring title_l = to_lower(title);

	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	std::wstring proc = cached_proc_image_name(pid);

	auto push = [&](ss_rect_kind kind) {
		ss_shared_rect r;
		r.kind = kind;
		r.screen.x = (double)rc.left;
		r.screen.y = (double)rc.top;
		r.screen.w = (double)w;
		r.screen.h = (double)h;
		ctx->rects.push_back(r);
	};

	/* Toast: process AND class signature, then the geometry gate
	 * (M6, SPEC 2.2). Gate-failed windows are NOT dropped — they fall
	 * through to block/allowlist matching like any other window, the
	 * gate only removes the toast-card over-masking. With no monitor
	 * data the gate classifies as toast (over-mask-safe). */
	if (proc == TOAST_PROC && cls_l == TOAST_CLASS) {
		ss_rect win;
		win.x = (double)rc.left;
		win.y = (double)rc.top;
		win.w = (double)w;
		win.h = (double)h;
		if (ss_toast_geom_plausible_any(ctx->mons, ctx->num_mons, &win)) {
			push(SS_RECT_TOAST);
			return TRUE;
		}
	}

	/* Blocklist: entry matches process image name OR title substring. */
	for (const std::wstring &entry : *ctx->blocklist) {
		if (entry.empty())
			continue;
		if ((!proc.empty() && proc.find(entry) != std::wstring::npos) ||
		    (!title_l.empty() && title_l.find(entry) != std::wstring::npos)) {
			push(SS_RECT_WINDOW);
			break;
		}
	}
	return TRUE;
}

/* ---- UIA focus-changed handler (password fields only) --------------- */

class FocusHandler : public IUIAutomationFocusChangedEventHandler {
	LONG ref_ = 1;

public:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override
	{
		if (riid == __uuidof(IUnknown) || riid == __uuidof(IUIAutomationFocusChangedEventHandler)) {
			*ppv = static_cast<IUIAutomationFocusChangedEventHandler *>(this);
			AddRef();
			return S_OK;
		}
		*ppv = nullptr;
		return E_NOINTERFACE;
	}
	ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&ref_); }
	ULONG STDMETHODCALLTYPE Release() override
	{
		LONG r = InterlockedDecrement(&ref_);
		if (r == 0)
			delete this;
		return r;
	}

	/* Minimal work per CLAUDE.md: read IsPassword, copy rect, store,
	 * return. No enumeration, no blocking. */
	HRESULT STDMETHODCALLTYPE HandleFocusChangedEvent(IUIAutomationElement *sender) override
	{
		if (!sender)
			return S_OK;
		BOOL is_pw = FALSE;
		if (FAILED(sender->get_CurrentIsPassword(&is_pw)))
			is_pw = FALSE;

		std::lock_guard<std::mutex> lk(g_pw_mutex);
		if (is_pw) {
			RECT rc = {0, 0, 0, 0};
			if (SUCCEEDED(sender->get_CurrentBoundingRectangle(&rc)) &&
			    (rc.right - rc.left) > 0 && (rc.bottom - rc.top) > 0) {
				g_pw_rect = rc;
				g_pw_valid = true;
			} else {
				g_pw_valid = false;
			}
		} else {
			g_pw_valid = false;
		}
		return S_OK;
	}
};

/* ---- watcher thread ------------------------------------------------- */

void publish_tick(EnumCtx &ctx)
{
	ss_snapshot snap;
	memset(&snap, 0, sizeof(snap));

	{
		std::lock_guard<std::mutex> lk(g_cfg_mutex);
		ctx.blocklist = &g_blocklist;
		ctx.rects.clear();
		ctx.num_mons = 0;
		ctx.mons_complete = true;
		if (!EnumDisplayMonitors(nullptr, nullptr, mon_enum_proc, reinterpret_cast<LPARAM>(&ctx)) ||
		    !ctx.mons_complete) {
			/* Incomplete monitor data must not flip the gate's failure
			 * direction: a toast on an unlisted monitor would fail the
			 * overlap test and go UNMASKED. Present the gate with an
			 * empty list instead — it then classifies every signature
			 * match as a toast (over-mask, iron rule 1 direction). */
			ctx.num_mons = 0;
		}
		pid_cache_begin_tick();
		EnumWindows(enum_proc, reinterpret_cast<LPARAM>(&ctx));
		pid_cache_end_tick();
	}

	/* Append the current password-field rect, if any. */
	{
		std::lock_guard<std::mutex> lk(g_pw_mutex);
		if (g_pw_valid && ctx.rects.size() < SS_MAX_RECTS) {
			ss_shared_rect r;
			r.kind = SS_RECT_FIELD;
			r.screen.x = (double)g_pw_rect.left;
			r.screen.y = (double)g_pw_rect.top;
			r.screen.w = (double)(g_pw_rect.right - g_pw_rect.left);
			r.screen.h = (double)(g_pw_rect.bottom - g_pw_rect.top);
			ctx.rects.push_back(r);
		}
	}

	size_t n = std::min(ctx.rects.size(), (size_t)SS_MAX_RECTS);
	snap.num_rects = n;
	for (size_t i = 0; i < n; i++)
		snap.rects[i] = ctx.rects[i];
	snap.heartbeat_ns = os_gettime_ns();
	ss_state_publish(&snap);
}

DWORD WINAPI watcher_thread(LPVOID)
{
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	bool com_ok = SUCCEEDED(hr);

	IUIAutomation *uia = nullptr;
	FocusHandler *handler = nullptr;
	if (com_ok) {
		if (SUCCEEDED(CoCreateInstance(__uuidof(CUIAutomation), nullptr, CLSCTX_INPROC_SERVER,
					       __uuidof(IUIAutomation), reinterpret_cast<void **>(&uia))) &&
		    uia) {
			handler = new FocusHandler();
			if (FAILED(uia->AddFocusChangedEventHandler(nullptr, handler))) {
				obs_log(LOG_WARNING, "watcher: UIA focus handler registration failed; "
						     "password-field guard inactive this session");
			}
		} else {
			obs_log(LOG_WARNING, "watcher: UIA unavailable; password-field guard inactive");
		}
	} else {
		obs_log(LOG_ERROR, "watcher: CoInitializeEx(MTA) failed (0x%08lx)", (unsigned long)hr);
	}

	obs_log(LOG_INFO, "watcher thread started (tick %lums)", (unsigned long)WATCH_TICK_MS);

	EnumCtx ctx;
	ctx.rects.reserve(SS_MAX_RECTS);

#ifdef STREAMSENTRY_PERF_LOG
	/* 200-tick window (30s at 150ms): avg + max + p99 per SPEC 2.1. */
	static uint64_t perf_durs[200];
	size_t perf_n = 0;
#endif

	for (;;) {
		if (InterlockedCompareExchange(&g_killed, 0, 0) == 0) {
			uint64_t t0 = os_gettime_ns();
			publish_tick(ctx);
			uint64_t dur = os_gettime_ns() - t0;

			/* Always compiled (SPEC 2.1): a slow tick is the precursor
			 * of a stale-heartbeat fail-closed; surface it in the OBS
			 * log while it is still only a near-miss. */
			if (dur > TICK_WARN_NS)
				obs_log(LOG_WARNING,
					"watcher tick took %llu ms (early warning; "
					"fail-closed stale threshold is 500 ms)",
					(unsigned long long)(dur / 1000000ULL));

#ifdef STREAMSENTRY_PERF_LOG
			perf_durs[perf_n++] = dur;
			if (perf_n >= 200) {
				uint64_t sorted[200];
				memcpy(sorted, perf_durs, sizeof(sorted));
				std::sort(sorted, sorted + 200);
				uint64_t sum = 0;
				for (size_t i = 0; i < 200; i++)
					sum += sorted[i];
				double avg_ms = (double)sum / 200.0 / 1e6;
				double p99_ms = (double)sorted[197] / 1e6;
				double max_ms = (double)sorted[199] / 1e6;
				uint64_t lookups = g_cache_hits + g_cache_misses;
				obs_log(LOG_INFO,
					"PERF watcher tick: avg %.3f / p99 %.3f / max %.3f ms "
					"over 200 ticks (~%.2f%% of one core at %lums cadence); "
					"pid-cache %.1f%% hit (%llu lookups, %llu evictions)",
					avg_ms, p99_ms, max_ms,
					avg_ms / (double)WATCH_TICK_MS * 100.0, (unsigned long)WATCH_TICK_MS,
					lookups ? 100.0 * (double)g_cache_hits / (double)lookups : 0.0,
					(unsigned long long)lookups, (unsigned long long)g_cache_evictions);
				perf_n = 0;
				g_cache_hits = g_cache_misses = g_cache_evictions = 0;
			}
#endif
		}
		/* When killed we intentionally neither publish nor beat the
		 * heartbeat, so the render side fails closed within 500ms. */
		if (WaitForSingleObject(g_stop_event, WATCH_TICK_MS) == WAIT_OBJECT_0)
			break;
	}

	if (uia) {
		uia->RemoveAllEventHandlers();
		uia->Release();
	}
	if (handler)
		handler->Release();
	if (com_ok)
		CoUninitialize();

	obs_log(LOG_INFO, "watcher thread stopped");
	return 0;
}

void load_default_blocklist()
{
	std::lock_guard<std::mutex> lk(g_cfg_mutex);
	g_blocklist.clear();
	for (const wchar_t *e : DEFAULT_BLOCKLIST)
		g_blocklist.push_back(e);
}

} // namespace

/* ---- C interface ---------------------------------------------------- */

void ss_watcher_start(void)
{
	std::lock_guard<std::mutex> lk(g_mutex);
	if (g_refcount++ > 0)
		return; /* already running */

	if (g_blocklist.empty())
		load_default_blocklist();

	g_killed = 0;
	g_stop_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	/* SPEC 2.1: thread priority is the approved SECOND lever and only
	 * after measurement — raise to THREAD_PRIORITY_ABOVE_NORMAL solely
	 * if tick p99 under streaming load still approaches the 500ms stale
	 * threshold with the PID-name cache in place. Not applied yet. */
	g_thread = CreateThread(nullptr, 0, watcher_thread, nullptr, 0, nullptr);
	if (!g_thread) {
		obs_log(LOG_ERROR, "watcher: CreateThread failed; detection unavailable "
				   "(render side will fail closed)");
		if (g_stop_event) {
			CloseHandle(g_stop_event);
			g_stop_event = nullptr;
		}
		g_refcount = 0;
	}
}

void ss_watcher_stop(void)
{
	HANDLE thread = nullptr, stop = nullptr;
	{
		std::lock_guard<std::mutex> lk(g_mutex);
		if (g_refcount == 0)
			return;
		if (--g_refcount > 0)
			return;
		thread = g_thread;
		stop = g_stop_event;
		g_thread = nullptr;
		g_stop_event = nullptr;
	}
	if (stop)
		SetEvent(stop);
	if (thread) {
		WaitForSingleObject(thread, 3000);
		CloseHandle(thread);
	}
	if (stop)
		CloseHandle(stop);
}

void ss_watcher_set_blocklist(const char *multiline_utf8)
{
	std::lock_guard<std::mutex> lk(g_cfg_mutex);
	g_blocklist.clear();
	if (!multiline_utf8 || !*multiline_utf8) {
		for (const wchar_t *e : DEFAULT_BLOCKLIST)
			g_blocklist.push_back(e);
		return;
	}
	/* UTF-8 -> UTF-16, split on newlines, trim, lowercase. */
	int wlen = MultiByteToWideChar(CP_UTF8, 0, multiline_utf8, -1, nullptr, 0);
	std::wstring all;
	if (wlen > 0) {
		all.resize(wlen);
		MultiByteToWideChar(CP_UTF8, 0, multiline_utf8, -1, &all[0], wlen);
		if (!all.empty() && all.back() == L'\0')
			all.pop_back();
	}
	size_t start = 0;
	while (start <= all.size()) {
		size_t nl = all.find_first_of(L"\r\n", start);
		std::wstring line = all.substr(start, (nl == std::wstring::npos ? all.size() : nl) - start);
		size_t b = line.find_first_not_of(L" \t");
		size_t e = line.find_last_not_of(L" \t");
		if (b != std::wstring::npos)
			g_blocklist.push_back(to_lower(line.substr(b, e - b + 1)));
		if (nl == std::wstring::npos)
			break;
		start = nl + 1;
	}
	if (g_blocklist.empty())
		for (const wchar_t *e : DEFAULT_BLOCKLIST)
			g_blocklist.push_back(e);
}

const char *ss_watcher_default_blocklist_text(void)
{
	/* Newline-joined DEFAULT_BLOCKLIST, built once. */
	static std::string text;
	if (text.empty()) {
		for (const wchar_t *e : DEFAULT_BLOCKLIST) {
			std::wstring w(e);
			int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
			if (n > 1) {
				std::string s(n - 1, '\0');
				WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], n, nullptr, nullptr);
				text += s;
				text += '\n';
			}
		}
	}
	return text.c_str();
}

void ss_watcher_debug_set_killed(bool killed)
{
	InterlockedExchange(&g_killed, killed ? 1 : 0);
	if (killed)
		obs_log(LOG_WARNING, "watcher: DEBUG kill engaged (heartbeat frozen)");
	else
		obs_log(LOG_INFO, "watcher: DEBUG kill released");
}

bool ss_watcher_is_running(void)
{
	std::lock_guard<std::mutex> lk(g_mutex);
	return g_refcount > 0 && g_thread != nullptr;
}
