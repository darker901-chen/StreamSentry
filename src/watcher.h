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

/* The built-in default blocklist as newline-separated UTF-8 text, for
 * pre-filling the settings box. Points to static storage. */
const char *ss_watcher_default_blocklist_text(void);

/* M2 fault-injection hook (developer-only, for the fail-closed test):
 * when killed, the watcher loop stops publishing AND stops updating the
 * heartbeat, exactly as a dead thread would look to the render side,
 * without the hazards of TerminateThread. */
void ss_watcher_debug_set_killed(bool killed);

bool ss_watcher_is_running(void);

#ifdef __cplusplus
}
#endif
