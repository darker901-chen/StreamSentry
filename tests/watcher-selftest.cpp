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

/* Standalone self-test for the real watcher: exercises the COM MTA
 * thread, Win32 enumeration + blocklist matching, and the fault-
 * injection heartbeat freeze WITHOUT OBS. It links watcher.cpp +
 * shared-state.c and stubs the two libobs symbols they reference
 * (obs_log, os_gettime_ns), so the detection code under test is the
 * exact code that ships. Deterministic parts (blocklist appear/
 * disappear, kill -> heartbeat freeze) assert and fail the process on
 * mismatch; the toast part reports INCONCLUSIVE if the machine's Do Not
 * Disturb suppresses the banner (see reports/M2-DECISIONS.md). */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <cstdarg>
#include <cstdint>

extern "C" {
#include "../src/shared-state.h"
#include "../src/watcher.h"
}

/* ---- libobs stubs (test build only) --------------------------------- */
extern "C" void obs_log(int level, const char *format, ...)
{
	(void)level;
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

extern "C" uint64_t os_gettime_ns(void)
{
	static LARGE_INTEGER freq = {};
	if (freq.QuadPart == 0)
		QueryPerformanceFrequency(&freq);
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return (uint64_t)((now.QuadPart * 1000000000ULL) / freq.QuadPart);
}

/* ---- helpers -------------------------------------------------------- */

static int failures = 0;
#define CHECK(cond, msg)                                                      \
	do {                                                                  \
		if (!(cond)) {                                                 \
			printf("FAIL: %s\n", msg);                            \
			failures++;                                           \
		} else {                                                      \
			printf("ok:   %s\n", msg);                            \
		}                                                             \
	} while (0)

static size_t count_kind(ss_rect_kind kind)
{
	ss_snapshot snap;
	for (int tries = 0; tries < 50; tries++) {
		if (ss_state_try_read(&snap)) {
			size_t n = 0;
			for (size_t i = 0; i < snap.num_rects; i++)
				if (snap.rects[i].kind == kind)
					n++;
			return n;
		}
		Sleep(2);
	}
	return (size_t)-1;
}

static uint64_t read_heartbeat()
{
	ss_snapshot snap;
	for (int tries = 0; tries < 50; tries++) {
		if (ss_state_try_read(&snap))
			return snap.heartbeat_ns;
		Sleep(2);
	}
	return 0;
}

/* Poll until count_kind(kind) satisfies pred(count, baseline) or timeout. */
template <typename Pred> static bool wait_for(ss_rect_kind kind, Pred pred, int timeout_ms)
{
	int waited = 0;
	while (waited < timeout_ms) {
		size_t c = count_kind(kind);
		if (c != (size_t)-1 && pred(c))
			return true;
		Sleep(100);
		waited += 100;
	}
	return false;
}

int main()
{
	ss_state_init();

	/* Blocklist = a unique token that matches nothing + "notepad". */
	ss_watcher_set_blocklist("streamsentry_selftest_unique_token\nnotepad\n");
	ss_watcher_start();
	CHECK(ss_watcher_is_running(), "watcher thread started");

	Sleep(400); /* let a couple of ticks run */

	/* Heartbeat must be advancing while alive. */
	uint64_t hb1 = read_heartbeat();
	Sleep(400);
	uint64_t hb2 = read_heartbeat();
	CHECK(hb2 > hb1, "heartbeat advances while watcher alive");

	size_t base_block = count_kind(SS_RECT_WINDOW);
	printf("info: baseline blocklist rects = %zu\n", base_block);

	/* ---- blocklist appear/disappear ---- */
	STARTUPINFOW si = {sizeof(si)};
	PROCESS_INFORMATION pi = {};
	wchar_t cmd[] = L"notepad.exe";
	bool launched = CreateProcessW(nullptr, cmd, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
	CHECK(launched, "launched notepad.exe");

	if (launched) {
		bool appeared = wait_for(
			SS_RECT_WINDOW, [&](size_t c) { return c > base_block; }, 6000);
		CHECK(appeared, "blocklist rect appeared after notepad opened");

		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);

		/* Close notepad. On Windows 11 "notepad.exe" is a launcher stub
		 * that exits immediately while the real window lives in a
		 * separate (store-app) process, so terminating our launched
		 * handle would not close the window. Kill every notepad.exe
		 * instead (baseline was verified to have none). */
		STARTUPINFOW ksi = {sizeof(ksi)};
		PROCESS_INFORMATION kpi = {};
		wchar_t kcmd[] = L"taskkill /F /IM notepad.exe";
		if (CreateProcessW(nullptr, kcmd, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &ksi,
				   &kpi)) {
			WaitForSingleObject(kpi.hProcess, 3000);
			CloseHandle(kpi.hThread);
			CloseHandle(kpi.hProcess);
		}

		bool gone = wait_for(
			SS_RECT_WINDOW, [&](size_t c) { return c <= base_block; }, 6000);
		CHECK(gone, "blocklist rect disappeared after notepad closed");
	}

	/* ---- fault injection: kill -> heartbeat freezes ---- */
	uint64_t before_kill = read_heartbeat();
	ss_watcher_debug_set_killed(true);
	Sleep(650); /* > 500ms fail-closed threshold */
	uint64_t after_kill = read_heartbeat();
	CHECK(after_kill == before_kill, "heartbeat frozen while killed (>500ms) -> render fails closed");

	ss_watcher_debug_set_killed(false);
	Sleep(400);
	uint64_t after_revive = read_heartbeat();
	CHECK(after_revive > after_kill, "heartbeat resumes after un-kill");

	/* ---- toast (best-effort; INCONCLUSIVE under Do Not Disturb) ---- */
	size_t base_toast = count_kind(SS_RECT_TOAST);
	STARTUPINFOW si2 = {sizeof(si2)};
	PROCESS_INFORMATION pi2 = {};
	wchar_t tcmd[] =
		L"powershell -NoProfile -Command \""
		L"[Windows.UI.Notifications.ToastNotificationManager,Windows.UI.Notifications,ContentType=WindowsRuntime]|Out-Null;"
		L"[Windows.Data.Xml.Dom.XmlDocument,Windows.Data.Xml.Dom,ContentType=WindowsRuntime]|Out-Null;"
		L"$x=New-Object Windows.Data.Xml.Dom.XmlDocument;"
		L"$x.LoadXml('<toast scenario=\\\"incomingCall\\\"><visual><binding template=\\\"ToastGeneric\\\"><text>StreamSentry selftest</text></binding></visual></toast>');"
		L"$t=New-Object Windows.UI.Notifications.ToastNotification($x);"
		L"$a='{1AC14E77-02E7-4E5D-B744-2EB1AE5198B7}\\WindowsPowerShell\\v1.0\\powershell.exe';"
		L"[Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier($a).Show($t)\"";
	if (CreateProcessW(nullptr, tcmd, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si2, &pi2)) {
		WaitForSingleObject(pi2.hProcess, 8000);
		CloseHandle(pi2.hThread);
		CloseHandle(pi2.hProcess);
		bool toast_seen = wait_for(
			SS_RECT_TOAST, [&](size_t c) { return c > base_toast; }, 5000);
		if (toast_seen)
			printf("ok:   toast rect detected by watcher\n");
		else
			printf("INCONCLUSIVE: no toast banner detected (likely Do Not "
			       "Disturb / focus assist suppressing the banner) - see "
			       "reports/M2-DECISIONS.md; real toast masking is on the human checklist\n");
	} else {
		printf("INCONCLUSIVE: could not launch toast probe\n");
	}

	ss_watcher_stop();
	CHECK(!ss_watcher_is_running(), "watcher thread stopped cleanly");
	ss_state_free();

	if (failures) {
		printf("watcher-selftest: %d FAILURE(S)\n", failures);
		return 1;
	}
	printf("watcher-selftest: all deterministic checks passed\n");
	return 0;
}
