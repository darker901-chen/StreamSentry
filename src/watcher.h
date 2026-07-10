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

/* The detection watcher: one COM MTA thread that owns all OS-level
 * detection (Win32 window enumeration for toasts + blocklist, UIA
 * focus-changed events for password fields) and publishes screen-space
 * rects + a heartbeat into shared state. C interface; the implementation
 * is C++ (watcher.cpp) for sane COM/UIA usage.
 *
 * Refcounted start/stop so multiple filter instances share one thread. */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Reference-counted: first start launches the thread, last stop joins
 * it. Safe to call from OBS module load and per-filter create/destroy. */
void ss_watcher_start(void);
void ss_watcher_stop(void);

/* Replace the blocklist (newline-separated process names / title
 * substrings, UTF-8). NULL or empty clears to the built-in defaults.
 * Thread-safe. */
void ss_watcher_set_blocklist(const char *multiline_utf8);

/* M7 (SPEC 2.3): select the matching mode. Like the blocklist this is
 * process-global (one shared watcher); with multiple filter instances
 * the most recently updated one wins (documented). Thread-safe. */
void ss_watcher_set_mode_allowlist(bool allowlist);

/* Replace the allowlist (same format/matching as the blocklist). NULL
 * or empty clears to EMPTY — an empty allowlist masks everything;
 * there are no built-in approvals (SPEC 2.3). Thread-safe. */
void ss_watcher_set_allowlist(const char *multiline_utf8);

/* The built-in default blocklist as newline-separated UTF-8 text, for
 * pre-filling the settings box. Points to static storage. */
const char *ss_watcher_default_blocklist_text(void);

/* M8 picker support (SPEC 2.5): one-shot enumeration of currently
 * visible top-level windows, using the same visibility/cloak gates as
 * detection, deduplicated by process image name. Runs on the calling
 * thread (UI); independent of the watcher thread and its caches.
 * Returns the number of entries written (at most max_count). */
struct ss_open_window {
	char proc[64];   /* lowercase process image name, UTF-8 */
	char title[128]; /* a representative window title, UTF-8; may be empty */
};
size_t ss_enum_open_windows(struct ss_open_window *out, size_t max_count);

/* M2 fault-injection hook (developer-only, for the degraded-render test):
 * when killed, the watcher loop stops publishing AND stops updating the
 * heartbeat, exactly as a dead thread would look to the render side,
 * without the hazards of TerminateThread. */
void ss_watcher_debug_set_killed(bool killed);

bool ss_watcher_is_running(void);

#ifdef __cplusplus
}
#endif
