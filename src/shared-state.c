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

#include "shared-state.h"

#include <string.h>

#include <util/threading.h>

static pthread_mutex_t state_mutex;
static struct ss_snapshot state_snap; /* guarded by state_mutex */
static bool state_ready = false;

void ss_state_init(void)
{
	pthread_mutex_init_value(&state_mutex);
	pthread_mutex_init(&state_mutex, NULL);
	memset(&state_snap, 0, sizeof(state_snap));
	state_ready = true;
}

void ss_state_free(void)
{
	if (state_ready) {
		pthread_mutex_destroy(&state_mutex);
		state_ready = false;
	}
}

void ss_state_publish(const struct ss_snapshot *snap)
{
	if (!state_ready || !snap)
		return;
	pthread_mutex_lock(&state_mutex);
	state_snap = *snap;
	pthread_mutex_unlock(&state_mutex);
}

void ss_state_touch_heartbeat(uint64_t now_ns)
{
	if (!state_ready)
		return;
	pthread_mutex_lock(&state_mutex);
	state_snap.heartbeat_ns = now_ns;
	pthread_mutex_unlock(&state_mutex);
}

bool ss_state_try_read(struct ss_snapshot *out)
{
	if (!state_ready || !out)
		return false;
	if (pthread_mutex_trylock(&state_mutex) != 0)
		return false;
	*out = state_snap;
	pthread_mutex_unlock(&state_mutex);
	return true;
}
