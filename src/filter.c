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
#include <util/threading.h>
#include <graphics/vec4.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "filter.h"
#include "coord-map.h"
#include "frame-decide.h"
#include "plate-gen.h"
#include "shared-state.h"
#include "geom-resolve.h"
#include "watcher.h"

/* Staleness threshold (SPEC.md): heartbeat older than 500ms means the
 * detection side cannot be trusted. Per the 2026-07-09 owner ruling
 * (SPEC 2.7) that no longer blacks the output — the source renders
 * unmodified and a status chip tells the user protection is inactive.
 * The threshold is deliberately not user-configurable. */
#define SS_HEARTBEAT_STALE_NS 500000000ULL

/* Padding in source pixels around CONFIDENT detections (SPEC.md: 8-16,
 * must stay above ss_plate_max_corner_inset()). Placement tolerance,
 * not guess-masking. */
#define SS_MASK_PAD_PX 12.0

enum ss_render_mode {
	SS_MODE_INIT = 0,
	SS_MODE_NORMAL,
	SS_MODE_DEGRADED, /* rendering normally, protection unverified */
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
	bool mode_allowlist; /* M7 (SPEC 2.3): failure direction + matching */

	bool last_mask_all; /* transition logging */

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

#ifdef STREAMSENTRY_PERF_LOG
	uint64_t perf_accum_ns;
	uint64_t perf_samples;
#endif
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
	f->mode_allowlist = strcmp(obs_data_get_string(settings, "mode"), "allowlist") == 0;

	/* The watcher is a single shared thread, so lists and mode are
	 * global: with multiple StreamSentry filters the most-recently-
	 * updated one wins (documented). Empty blocklist falls back to the
	 * built-in defaults; empty allowlist approves nothing (SPEC 2.3). */
	ss_watcher_set_mode_allowlist(f->mode_allowlist);
	ss_watcher_set_blocklist(obs_data_get_string(settings, "blocklist"));
	ss_watcher_set_allowlist(obs_data_get_string(settings, "allowlist"));
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
	free_textures(f);
	ss_watcher_stop();
	bfree(f);
}

/* ---- window picker (M8, SPEC 2.5) ------------------------------------ */

/* Case-insensitive "is this exact line already in the multiline list". */
static bool list_has_line(const char *text, const char *line)
{
	if (!text || !line || !*line)
		return false;
	size_t line_len = strlen(line);
	const char *p = text;
	while (*p) {
		const char *end = p;
		while (*end && *end != '\n' && *end != '\r')
			end++;
		/* trim */
		const char *b = p;
		const char *e = end;
		while (b < e && (*b == ' ' || *b == '\t'))
			b++;
		while (e > b && (e[-1] == ' ' || e[-1] == '\t'))
			e--;
		if ((size_t)(e - b) == line_len) {
			size_t i;
			for (i = 0; i < line_len; i++) {
				if (tolower((unsigned char)b[i]) != tolower((unsigned char)line[i]))
					break;
			}
			if (i == line_len)
				return true;
		}
		p = (*end) ? end + 1 : end;
	}
	return false;
}

static void picker_fill_combo(obs_property_t *combo)
{
	obs_property_list_clear(combo);
	struct ss_open_window wins[SS_MAX_RECTS];
	size_t n = ss_enum_open_windows(wins, SS_MAX_RECTS);
	for (size_t i = 0; i < n; i++) {
		char label[200];
		snprintf(label, sizeof(label), "%s%s%s", wins[i].proc, wins[i].title[0] ? " - " : "",
			 wins[i].title);
		obs_property_list_add_string(combo, label, wins[i].proc);
	}
}

static bool picker_refresh_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	UNUSED_PARAMETER(p);
	UNUSED_PARAMETER(data);
	obs_property_t *combo = obs_properties_get(props, "picker_window");
	if (combo)
		picker_fill_combo(combo);
	return true;
}

/* Append the selected process name to the ACTIVE mode's list (SPEC
 * 2.5): no hand-typing, duplicates are not added twice, the textbox
 * stays for power users. */
static bool picker_add_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	struct ss_filter *f = data;
	if (!f)
		return false;

	obs_data_t *settings = obs_source_get_settings(f->source);
	const char *proc = obs_data_get_string(settings, "picker_window");
	if (proc && *proc) {
		const char *key = (strcmp(obs_data_get_string(settings, "mode"), "allowlist") == 0) ? "allowlist"
													: "blocklist";
		const char *cur = obs_data_get_string(settings, key);
		if (!list_has_line(cur, proc)) {
			size_t need = strlen(cur) + strlen(proc) + 2;
			char *joined = bmalloc(need);
			if (cur[0])
				snprintf(joined, need, "%s\n%s", cur, proc);
			else
				snprintf(joined, need, "%s", proc);
			obs_data_set_string(settings, key, joined);
			bfree(joined);
			obs_source_update(f->source, settings);
		}
	}
	obs_data_release(settings);
	return true; /* refresh so the textbox shows the appended line */
}

/* Show only the active mode's list (SPEC 2.3: separate storage — a
 * mode switch must never reinterpret one list as the other). */
static bool mode_modified(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	bool allow = strcmp(obs_data_get_string(settings, "mode"), "allowlist") == 0;
	obs_property_set_visible(obs_properties_get(props, "blocklist"), !allow);
	obs_property_set_visible(obs_properties_get(props, "allowlist"), allow);
	return true;
}

static obs_properties_t *filter_get_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();
	obs_properties_add_bool(props, "enabled", obs_module_text("Enable"));

	obs_property_t *mode =
		obs_properties_add_list(props, "mode", obs_module_text("Mode"), OBS_COMBO_TYPE_LIST,
					OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(mode, obs_module_text("ModeBlocklist"), "blocklist");
	obs_property_list_add_string(mode, obs_module_text("ModeAllowlist"), "allowlist");
	obs_property_set_modified_callback(mode, mode_modified);

	/* One process name or window-title substring per line (SPEC
	 * settings UI). There is deliberately NO option to disable the
	 * protection-degraded status chip. */
	obs_property_t *bl =
		obs_properties_add_text(props, "blocklist", obs_module_text("Blocklist"), OBS_TEXT_MULTILINE);
	obs_property_set_long_description(bl, obs_module_text("BlocklistHint"));
	obs_property_t *al =
		obs_properties_add_text(props, "allowlist", obs_module_text("Allowlist"), OBS_TEXT_MULTILINE);
	obs_property_set_long_description(al, obs_module_text("AllowlistHint"));

	/* Window picker (M8, SPEC 2.5): pre-filled on dialog open; the
	 * refresh button re-enumerates; add appends to the ACTIVE list. */
	obs_property_t *combo = obs_properties_add_list(props, "picker_window", obs_module_text("PickerWindow"),
							OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_set_long_description(combo, obs_module_text("PickerWindowHint"));
	picker_fill_combo(combo);
	obs_property_t *refresh = obs_properties_add_button(props, "picker_refresh",
							    obs_module_text("PickerRefresh"),
							    picker_refresh_clicked);
	obs_property_set_long_description(refresh, obs_module_text("PickerRefreshHint"));
	obs_property_t *add = obs_properties_add_button2(props, "picker_add", obs_module_text("PickerAdd"),
							 picker_add_clicked, data);
	obs_property_set_long_description(add, obs_module_text("PickerAddHint"));
	return props;
}

static void filter_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "enabled", true);
	/* Blocklist mode is the default: v0.1 behavior preserved on
	 * upgrade (SPEC 2.3). Allowlist defaults to empty = approve
	 * nothing. */
	obs_data_set_default_string(settings, "mode", "blocklist");
	obs_data_set_default_string(settings, "blocklist", ss_watcher_default_blocklist_text());
	obs_data_set_default_string(settings, "allowlist", "");
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

/* Full-source privacy plate (M7): drawn INSTEAD of the target for
 * allowlist mask-all (SPEC 2.3). Solid opaque fallback if the texture
 * cannot be created — a full-cover mask is never dropped (iron rule 3). */
static void draw_full_plate(struct ss_filter *f, uint32_t w, uint32_t h)
{
	gs_texture_t *tex = get_plate_texture(f, SS_RECT_WINDOW, bucket_dim((double)w), bucket_dim((double)h));
	if (tex) {
		draw_texture(tex, 0.0f, 0.0f, (float)w, (float)h);
	} else {
		struct vec4 dark;
		vec4_set(&dark, 0.08f, 0.09f, 0.11f, 1.0f);
		draw_solid(0.0f, 0.0f, (float)w, (float)h, &dark);
		obs_log(LOG_WARNING, "full plate texture allocation failed; solid fallback drawn");
	}
}

/* Small opaque status chip in the top-left corner of the normally
 * rendered output (owner ruling 2026-07-09 / SPEC 2.7): the user must
 * be told protection is inactive, without the output being disrupted.
 * Deliberately not user-disableable. */
static void draw_status_chip(struct ss_filter *f, uint32_t w, uint32_t h)
{
	if (!f->banner_tex) {
		struct ss_image img;
		if (ss_gen_status_banner(&img)) {
			f->banner_tex = tex_from_image(&img);
			f->banner_w = img.w;
			f->banner_h = img.h;
			ss_image_free(&img);
		}
	}
	if (!f->banner_tex || w < 160 || h < 60)
		return;

	float bw = (float)f->banner_w, bh = (float)f->banner_h;
	float max_w = (float)w * 0.4f;
	if (bw > max_w) {
		float scale = max_w / bw;
		bw *= scale;
		bh *= scale;
	}
	draw_texture(f->banner_tex, 12.0f, 12.0f, bw, bh);
}

static void log_mode(struct ss_filter *f, enum ss_render_mode mode, uint64_t age_ns, const char *reason)
{
	enum ss_render_mode prev = f->last_mode;
	if (mode == prev)
		return;
	f->last_mode = mode;
	if (mode == SS_MODE_DEGRADED) {
		obs_log(LOG_WARNING, "PROTECTION DEGRADED: %s (heartbeat age %llu ms) - rendering with status chip",
			reason, (unsigned long long)(age_ns / 1000000ULL));
	} else if (prev == SS_MODE_DEGRADED) {
		obs_log(LOG_INFO, "protection restored: full masking active again");
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
		 * UI) — not a failure state, so no chip. The chip cannot be
		 * disabled while the filter is enabled. */
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

	/* Geometry is resolved only when a fresh snapshot carries rects.
	 * Everything else — which rects get masks, whether the chip shows,
	 * and crucially that a degradation flag never drops a confident
	 * mask (guardian M6.5 V6) — is decided by the pure, unit-tested
	 * frame-decide module (SPEC 2.7 ordering). */
	struct ss_capture_geom geom;
	bool geom_valid = false;
	if (f->have_snap && age <= SS_HEARTBEAT_STALE_NS && f->snap.num_rects > 0)
		geom_valid = ss_resolve_capture_geom(target, &geom);

	struct ss_frame_decision dec;
	ss_decide_frame(&f->snap, f->have_snap, age, SS_HEARTBEAT_STALE_NS, f->mode_allowlist, geom_valid, &geom,
			SS_MASK_PAD_PX, &dec);

#ifdef STREAMSENTRY_PERF_LOG
	/* Per-frame CPU decision cost (state read + heartbeat + geometry +
	 * mapping) — the work StreamSentry adds that would not exist without
	 * it. GPU draw submission is excluded (it is libobs, not us).
	 * Compiled out of release builds. */
	f->perf_accum_ns += os_gettime_ns() - now;
	f->perf_samples++;
	if (f->perf_samples >= 600) {
		obs_log(LOG_INFO, "PERF render decision: avg %.2f us/frame over %llu frames",
			(double)f->perf_accum_ns / (double)f->perf_samples / 1000.0,
			(unsigned long long)f->perf_samples);
		f->perf_accum_ns = 0;
		f->perf_samples = 0;
	}
#endif

	/* Allowlist mask-all (SPEC 2.3) draws a full-source privacy plate
	 * INSTEAD of the target. The chip still stacks on top when protection
	 * is unverified, so the streamer keeps learning about failures. */
	if (dec.mask_all) {
		draw_full_plate(f, w, h);
		if (dec.unverified)
			draw_status_chip(f, w, h);
		if (dec.mask_all && !f->last_mask_all) {
			f->last_mask_all = true;
			obs_log(LOG_INFO, "mask-all engaged (%s)",
				f->mode_allowlist ? "allowlist default" : "rect overflow");
		}
		log_mode(f, dec.unverified ? SS_MODE_DEGRADED : SS_MODE_NORMAL, age == UINT64_MAX ? 0 : age,
			 dec.reason);
		return;
	}
	if (f->last_mask_all) {
		f->last_mask_all = false;
		obs_log(LOG_INFO, "mask-all released");
	}

	/* If the filter chain cannot be entered: blocklist mode falls back
	 * to skipping the filter — the source still shows — with the chip
	 * on top when masks were needed but undrawable (owner ruling: never
	 * disrupt the output). Allowlist mode with masks pending must NOT
	 * fail open (guardian M7 V2): the full-source plate needs no filter
	 * chain, so default-deny is drawable on this path too. */
	if (!obs_source_process_filter_begin(f->source, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) {
		if (dec.unverified || dec.num_mapped > 0) {
			if (f->mode_allowlist) {
				draw_full_plate(f, w, h);
				if (dec.unverified)
					draw_status_chip(f, w, h);
			} else {
				obs_source_skip_video_filter(f->source);
				draw_status_chip(f, w, h);
			}
			log_mode(f, SS_MODE_DEGRADED, age == UINT64_MAX ? 0 : age,
				 "filter chain bypassed with masks pending");
		} else {
			obs_source_skip_video_filter(f->source);
		}
		return;
	}
	obs_source_process_filter_end(f->source, obs_get_base_effect(OBS_EFFECT_DEFAULT), w, h);

	for (size_t i = 0; i < dec.num_mapped; i++) {
		uint32_t bw = bucket_dim(dec.mapped[i].w);
		uint32_t bh = bucket_dim(dec.mapped[i].h);
		gs_texture_t *tex = get_plate_texture(f, dec.kinds[i], bw, bh);
		if (tex) {
			draw_texture(tex, (float)dec.mapped[i].x, (float)dec.mapped[i].y, (float)dec.mapped[i].w,
				     (float)dec.mapped[i].h);
		} else {
			/* A confidently mapped threat is never left unmasked:
			 * solid opaque fallback (iron rule 3 as amended). */
			struct vec4 dark;
			vec4_set(&dark, 0.08f, 0.09f, 0.11f, 1.0f);
			draw_solid((float)dec.mapped[i].x, (float)dec.mapped[i].y, (float)dec.mapped[i].w,
				   (float)dec.mapped[i].h, &dark);
			obs_log(LOG_WARNING, "plate texture allocation failed; solid fallback drawn");
		}
	}

	if (dec.unverified) {
		draw_status_chip(f, w, h);
		log_mode(f, SS_MODE_DEGRADED, age == UINT64_MAX ? 0 : age, dec.reason);
	} else {
		log_mode(f, SS_MODE_NORMAL, age, "");
	}
	if (dec.num_mapped != f->last_plate_count) {
		obs_log(LOG_INFO, "mask plates active: %zu", dec.num_mapped);
		f->last_plate_count = dec.num_mapped;
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
