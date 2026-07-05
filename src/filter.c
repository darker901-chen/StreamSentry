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

#include <obs-module.h>
#include <plugin-support.h>
#include <util/platform.h>
#include <graphics/vec4.h>

#include <string.h>

#include "filter.h"
#include "coord-map.h"
#include "plate-gen.h"
#include "shared-state.h"
#include "geom-resolve.h"
#include "watcher.h"

/* Fail-closed threshold (SPEC.md): heartbeat older than 500ms means the
 * detection side cannot be trusted; the entire output goes black. This
 * constant is deliberately not exposed anywhere user-configurable. */
#define SS_HEARTBEAT_STALE_NS 500000000ULL

/* Over-mask padding in source pixels (SPEC.md: 8-16, must stay above
 * ss_plate_max_corner_inset()). */
#define SS_MASK_PAD_PX 12.0

enum ss_render_mode {
	SS_MODE_INIT = 0,
	SS_MODE_NORMAL,
	SS_MODE_FAIL_CLOSED,
};

#define SS_TEX_CACHE 8

struct ss_tex_entry {
	bool used;
	enum ss_rect_kind kind;
	uint32_t w, h;
	uint64_t last_use;
	gs_texture_t *tex;
};

struct ss_filter {
	obs_source_t *source;

	/* settings */
	bool enabled;
	bool debug_kill; /* developer-only fault injection; removed at M3 */

	/* last successfully read snapshot */
	struct ss_snapshot snap;
	bool have_snap;

	/* render-side caches */
	struct ss_tex_entry cache[SS_TEX_CACHE];
	gs_texture_t *banner_tex;
	uint32_t banner_w, banner_h;
	uint64_t frame_counter;

	/* state-transition logging (no per-frame spam) */
	enum ss_render_mode last_mode;
	size_t last_plate_count;
};

static const char *filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FilterName");
}

static void filter_update(void *data, obs_data_t *settings)
{
	struct ss_filter *f = data;
	f->enabled = obs_data_get_bool(settings, "enabled");

	bool kill = obs_data_get_bool(settings, "debug_kill");
	if (kill != f->debug_kill) {
		f->debug_kill = kill;
		ss_watcher_debug_set_killed(kill);
	}
}

static void *filter_create(obs_data_t *settings, obs_source_t *source)
{
	struct ss_filter *f = bzalloc(sizeof(struct ss_filter));
	f->source = source;
	f->last_mode = SS_MODE_INIT;
	ss_watcher_start();
	filter_update(f, settings);
	return f;
}

static void free_textures(struct ss_filter *f)
{
	obs_enter_graphics();
	for (size_t i = 0; i < SS_TEX_CACHE; i++) {
		if (f->cache[i].tex)
			gs_texture_destroy(f->cache[i].tex);
		f->cache[i].tex = NULL;
		f->cache[i].used = false;
	}
	if (f->banner_tex) {
		gs_texture_destroy(f->banner_tex);
		f->banner_tex = NULL;
	}
	obs_leave_graphics();
}

static void filter_destroy(void *data)
{
	struct ss_filter *f = data;
	/* Ensure a fault-injecting instance doesn't leave the shared watcher
	 * killed for other instances / next run. */
	if (f->debug_kill)
		ss_watcher_debug_set_killed(false);
	free_textures(f);
	ss_watcher_stop();
	bfree(f);
}

static obs_properties_t *filter_get_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();
	obs_properties_add_bool(props, "enabled", obs_module_text("Enable"));

	/* Developer-only fault injection, removed at M3. It can only FREEZE
	 * the watcher heartbeat (forcing fail-closed) — there is no toggle
	 * that can weaken or disable the black-out. */
	obs_properties_t *dev = obs_properties_create();
	obs_properties_add_bool(dev, "debug_kill", obs_module_text("DebugKill"));
	obs_properties_add_group(props, "developer", obs_module_text("DebugGroup"), OBS_GROUP_NORMAL, dev);
	return props;
}

static void filter_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "enabled", true);
	obs_data_set_default_bool(settings, "debug_kill", false);
}

/* ---- drawing helpers ------------------------------------------------ */

static void draw_solid(float x, float y, float w, float h, struct vec4 *color)
{
	gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_eparam_t *cparam = gs_effect_get_param_by_name(solid, "color");
	gs_effect_set_vec4(cparam, color);
	gs_matrix_push();
	gs_matrix_translate3f(x, y, 0.0f);
	while (gs_effect_loop(solid, "Solid"))
		gs_draw_sprite(NULL, 0, (uint32_t)w, (uint32_t)h);
	gs_matrix_pop();
}

static void draw_texture(gs_texture_t *tex, float x, float y, float w, float h)
{
	gs_effect_t *def = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	gs_eparam_t *img = gs_effect_get_param_by_name(def, "image");
	gs_effect_set_texture(img, tex);
	gs_matrix_push();
	gs_matrix_translate3f(x, y, 0.0f);
	while (gs_effect_loop(def, "Draw"))
		gs_draw_sprite(tex, 0, (uint32_t)w, (uint32_t)h);
	gs_matrix_pop();
}

static gs_texture_t *tex_from_image(const struct ss_image *img)
{
	const uint8_t *level = img->rgba;
	return gs_texture_create(img->w, img->h, GS_RGBA, 1, &level, 0);
}

static uint32_t bucket_dim(double v)
{
	uint32_t d = (uint32_t)(v + 0.5);
	uint32_t b = (d + 15) / 16 * 16;
	return b < 32 ? 32 : b;
}

static gs_texture_t *get_plate_texture(struct ss_filter *f, enum ss_rect_kind kind, uint32_t bw, uint32_t bh)
{
	enum ss_rect_kind style = (kind == SS_RECT_TOAST) ? SS_RECT_TOAST : SS_RECT_WINDOW;

	struct ss_tex_entry *victim = NULL;
	for (size_t i = 0; i < SS_TEX_CACHE; i++) {
		struct ss_tex_entry *e = &f->cache[i];
		if (e->used && e->kind == style && e->w == bw && e->h == bh) {
			e->last_use = f->frame_counter;
			return e->tex;
		}
		if (!victim || !e->used || (victim->used && e->used && e->last_use < victim->last_use))
			victim = e;
	}

	struct ss_image img;
	bool ok = (style == SS_RECT_TOAST) ? ss_gen_toast_card(bw, bh, &img) : ss_gen_privacy_plate(bw, bh, &img);
	if (!ok)
		return NULL;

	gs_texture_t *tex = tex_from_image(&img);
	ss_image_free(&img);
	if (!tex)
		return NULL;

	if (victim->tex)
		gs_texture_destroy(victim->tex);
	victim->used = true;
	victim->kind = style;
	victim->w = bw;
	victim->h = bh;
	victim->last_use = f->frame_counter;
	victim->tex = tex;
	return tex;
}

static void draw_fail_closed(struct ss_filter *f, uint32_t w, uint32_t h)
{
	struct vec4 black;
	vec4_set(&black, 0.0f, 0.0f, 0.0f, 1.0f);
	draw_solid(0.0f, 0.0f, (float)w, (float)h, &black);

	if (!f->banner_tex) {
		struct ss_image img;
		if (ss_gen_status_banner(&img)) {
			f->banner_tex = tex_from_image(&img);
			f->banner_w = img.w;
			f->banner_h = img.h;
			ss_image_free(&img);
		}
	}
	if (f->banner_tex && w >= 160 && h >= 60) {
		float bw = (float)f->banner_w, bh = (float)f->banner_h;
		if (bw > (float)w - 16.0f) {
			float scale = ((float)w - 16.0f) / bw;
			bw *= scale;
			bh *= scale;
		}
		draw_texture(f->banner_tex, ((float)w - bw) / 2.0f, ((float)h - bh) / 2.0f, bw, bh);
	}
}

static void log_mode(struct ss_filter *f, enum ss_render_mode mode, uint64_t age_ns, const char *reason)
{
	enum ss_render_mode prev = f->last_mode;
	if (mode == prev)
		return;
	f->last_mode = mode;
	if (mode == SS_MODE_FAIL_CLOSED) {
		obs_log(LOG_WARNING, "FAIL-CLOSED engaged: %s (heartbeat age %llu ms)", reason,
			(unsigned long long)(age_ns / 1000000ULL));
	} else if (prev == SS_MODE_FAIL_CLOSED) {
		obs_log(LOG_INFO, "fail-closed cleared: normal rendering resumed");
	}
	/* INIT -> NORMAL is silent: nothing was engaged, nothing cleared. */
}

static void filter_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	struct ss_filter *f = data;
	f->frame_counter++;

	obs_source_t *target = obs_filter_get_target(f->source);
	uint32_t w = target ? obs_source_get_base_width(target) : 0;
	uint32_t h = target ? obs_source_get_base_height(target) : 0;

	if (!f->enabled) {
		/* Whole feature off by explicit user choice (SPEC settings
		 * UI). Fail-closed cannot be disabled while enabled. */
		obs_source_skip_video_filter(f->source);
		return;
	}

	if (!w || !h) {
		obs_source_skip_video_filter(f->source);
		return;
	}

	if (ss_state_try_read(&f->snap))
		f->have_snap = true;

	uint64_t now = os_gettime_ns();
	uint64_t age = (f->have_snap && now > f->snap.heartbeat_ns) ? now - f->snap.heartbeat_ns : UINT64_MAX;
	bool unhealthy = !f->have_snap || age > SS_HEARTBEAT_STALE_NS;
	const char *reason = !f->have_snap ? "no detection snapshot" : "heartbeat stale";

	/* Map every rect BEFORE rendering the target: a mapping failure must
	 * black the frame, not partially mask it. Geometry is resolved only
	 * when there is something to mask. */
	struct ss_rect mapped[SS_MAX_RECTS];
	enum ss_rect_kind kinds[SS_MAX_RECTS];
	size_t num_mapped = 0;

	if (!unhealthy && f->snap.num_rects > 0) {
		struct ss_capture_geom geom;
		if (!ss_resolve_capture_geom(target, &geom)) {
			unhealthy = true;
			reason = "capture geometry unresolved (unsupported source or scaled capture)";
		} else {
			for (size_t i = 0; i < f->snap.num_rects && !unhealthy; i++) {
				struct ss_rect out;
				enum ss_map_result r = ss_map_screen_rect(&geom, &f->snap.rects[i].screen,
									  SS_MASK_PAD_PX, &out);
				if (r == SS_MAP_OK) {
					mapped[num_mapped] = out;
					kinds[num_mapped] = f->snap.rects[i].kind;
					num_mapped++;
				} else if (r == SS_MAP_INVALID) {
					unhealthy = true;
					reason = "coordinate mapping failed";
				}
				/* NOT_VISIBLE: rect outside this capture, skip */
			}
		}
	}

	if (unhealthy) {
		draw_fail_closed(f, w, h);
		log_mode(f, SS_MODE_FAIL_CLOSED, age == UINT64_MAX ? 0 : age, reason);
		return;
	}

	if (!obs_source_process_filter_begin(f->source, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) {
		draw_fail_closed(f, w, h);
		log_mode(f, SS_MODE_FAIL_CLOSED, age, "filter chain bypassed");
		return;
	}
	obs_source_process_filter_end(f->source, obs_get_base_effect(OBS_EFFECT_DEFAULT), w, h);

	for (size_t i = 0; i < num_mapped; i++) {
		uint32_t bw = bucket_dim(mapped[i].w);
		uint32_t bh = bucket_dim(mapped[i].h);
		gs_texture_t *tex = get_plate_texture(f, kinds[i], bw, bh);
		if (!tex) {
			draw_fail_closed(f, w, h);
			log_mode(f, SS_MODE_FAIL_CLOSED, age, "plate texture allocation failed");
			return;
		}
		draw_texture(tex, (float)mapped[i].x, (float)mapped[i].y, (float)mapped[i].w, (float)mapped[i].h);
	}

	log_mode(f, SS_MODE_NORMAL, age, "");
	if (num_mapped != f->last_plate_count) {
		obs_log(LOG_INFO, "mask plates active: %zu", num_mapped);
		f->last_plate_count = num_mapped;
	}
}

struct obs_source_info streamsentry_filter_info = {
	.id = "streamsentry_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = filter_get_name,
	.create = filter_create,
	.destroy = filter_destroy,
	.update = filter_update,
	.get_properties = filter_get_properties,
	.get_defaults = filter_get_defaults,
	.video_render = filter_video_render,
};
