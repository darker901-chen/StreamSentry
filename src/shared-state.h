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

/* Shared state between the watcher (writer) and the render callback
 * (reader). Per the fixed architecture: rect list + heartbeat
 * timestamp; the render-side critical section is non-blocking (trylock)
 * and tiny (a copy of a fixed-size snapshot).
 *
 * The watcher produces SCREEN-space rects only; it cannot know which OBS
 * source/monitor the filter is attached to. The filter resolves its own
 * capture geometry each frame and maps these screen rects into source
 * space (see geom-resolve + coord-map). */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "coord-map.h"

#ifdef __cplusplus
extern "C" {
#endif

enum ss_rect_kind {
	SS_RECT_TOAST = 0,  /* notification-card mask style */
	SS_RECT_WINDOW = 1, /* privacy-plate mask style */
	SS_RECT_FIELD = 2,  /* privacy-plate mask style */
};

struct ss_shared_rect {
	enum ss_rect_kind kind;
	struct ss_rect screen; /* virtual-screen physical pixels */
};

#define SS_MAX_RECTS 64

struct ss_snapshot {
	uint64_t heartbeat_ns; /* os_gettime_ns() at last watcher tick */
	size_t num_rects;
	struct ss_shared_rect rects[SS_MAX_RECTS];
};

void ss_state_init(void);
void ss_state_free(void);

/* Writer side (watcher). Blocking lock, brief. */
void ss_state_publish(const struct ss_snapshot *snap);

/* Update only the heartbeat, keeping rects as-is. */
void ss_state_touch_heartbeat(uint64_t now_ns);

/* Reader side (render callback). Never blocks: on lock contention
 * returns false and the caller keeps using its previous snapshot —
 * staleness is still governed by the heartbeat check. */
bool ss_state_try_read(struct ss_snapshot *out);

#ifdef __cplusplus
}
#endif
