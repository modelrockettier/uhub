/*
 * uhub - A tiny ADC p2p connection hub
 * Copyright (C) 2014, Jan Vidar Krey
 * Copyright (C) 2020, Tim Schlueter
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "plugin_api/handle.h"
#include "util/memory.h"
#include "util/list.h"
#include "util/misc.h"
#include "util/log.h"
#include "util/config_token.h"

static inline void set_error_message(struct plugin_handle* plugin, const char* msg)
{
	plugin->error_msg = msg;
}

struct guest_pass_data
{
	char password[MAX_PASS_LEN + 1];
};

static struct guest_pass_data* parse_config(const char* line, struct plugin_handle* plugin)
{
	struct guest_pass_data* data = (struct guest_pass_data*) hub_malloc_zero(sizeof(*data));
	struct cfg_tokens* tokens = cfg_tokenize(line);
	char* token = cfg_token_get_first(tokens);

	if (!data)
	{
		set_error_message(plugin, "OOM");
		cfg_tokens_free(tokens);
		return NULL;
	}

	while (token)
	{
		struct cfg_settings* setting = cfg_settings_split(token);

		if (!setting)
		{
			set_error_message(plugin, "Unable to parse startup parameters");
			cfg_tokens_free(tokens);
			hub_free(data);
			return NULL;
		}

		if (strcmp(cfg_settings_get_key(setting), "password") == 0)
		{
			// Make sure it's possible to actually use the guest password
			if (strlen(cfg_settings_get_value(setting)) > MAX_PASS_LEN)
			{
				set_error_message(plugin, "Guest password too long");
				cfg_tokens_free(tokens);
				cfg_settings_free(setting);
				hub_free(data);
				return NULL;
			}

			strcpy(data->password, cfg_settings_get_value(setting));
		}
		else
		{
			set_error_message(plugin, "Unknown startup parameters given");
			cfg_tokens_free(tokens);
			cfg_settings_free(setting);
			hub_free(data);
			return NULL;
		}

		cfg_settings_free(setting);
		token = cfg_token_get_next(tokens);
	}
	cfg_tokens_free(tokens);

	if (strlen(data->password) == 0)
	{
		set_error_message(plugin, "No password is given, use password=<password>");
		hub_free(data);
		return NULL;
	}

	return data;
}

static plugin_st get_user(struct plugin_handle* plugin, const char* nickname, struct auth_info* auth_data)
{
	struct guest_pass_data* data = (struct guest_pass_data*) plugin->ptr;

	//printf("Guest auth attempt: nick '%s'\n", nickname);

	strlcpy(auth_data->nickname, nickname, MAX_NICK_LEN + 1);
	strlcpy(auth_data->password, data->password, MAX_PASS_LEN + 1);

	auth_data->credentials = auth_cred_guest;

	return st_allow;
}

int plugin_register(struct plugin_handle* plugin, const char* config)
{
	PLUGIN_INITIALIZE(plugin, "Guest password authentication plugin", "1.0", "Require a password for guest users.");
	set_error_message(plugin, NULL);

	// Authentication actions.
	plugin->funcs.auth_get_user = get_user;

	plugin->ptr = parse_config(config, plugin);
	if (plugin->ptr)
		return 0;
	return -1;
}

int plugin_unregister(struct plugin_handle* plugin)
{
	struct guest_pass_data* data;
	set_error_message(plugin, NULL);
	data = (struct guest_pass_data*) plugin->ptr;
	hub_free(data);
	return 0;
}

