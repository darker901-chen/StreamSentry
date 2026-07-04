/*
obsplugin
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

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

/* M0 skeleton: register a video filter that passes the source through
 * untouched. No detection, no masking. */

struct obsplugin_filter {
	obs_source_t *source;
	bool enabled;
};

static const char *obsplugin_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FilterName");
}

static void obsplugin_filter_update(void *data, obs_data_t *settings)
{
	struct obsplugin_filter *filter = data;
	filter->enabled = obs_data_get_bool(settings, "enabled");
}

static void *obsplugin_filter_create(obs_data_t *settings, obs_source_t *source)
{
	struct obsplugin_filter *filter = bzalloc(sizeof(struct obsplugin_filter));
	filter->source = source;
	obsplugin_filter_update(filter, settings);
	return filter;
}

static void obsplugin_filter_destroy(void *data)
{
	bfree(data);
}

static obs_properties_t *obsplugin_filter_get_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();
	obs_properties_add_bool(props, "enabled", obs_module_text("Enable"));
	return props;
}

static void obsplugin_filter_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "enabled", true);
}

static void obsplugin_filter_video_render(void *data, gs_effect_t *effect)
{
	struct obsplugin_filter *filter = data;

	UNUSED_PARAMETER(effect);
	obs_source_skip_video_filter(filter->source);
}

static struct obs_source_info obsplugin_filter_info = {
	.id = "obsplugin_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = obsplugin_filter_get_name,
	.create = obsplugin_filter_create,
	.destroy = obsplugin_filter_destroy,
	.update = obsplugin_filter_update,
	.get_properties = obsplugin_filter_get_properties,
	.get_defaults = obsplugin_filter_get_defaults,
	.video_render = obsplugin_filter_video_render,
};

bool obs_module_load(void)
{
	obs_register_source(&obsplugin_filter_info);
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}
