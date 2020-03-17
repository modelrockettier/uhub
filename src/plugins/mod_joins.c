/*
 * uhub - A tiny ADC p2p connection hub
 * Copyright (C) 2007-2012, Jan Vidar Krey
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "plugin_api/handle.h"
#include "util/cbuffer.h"
#include "util/config_token.h"
#include "util/log.h"
#include "util/memory.h"
#include "util/misc.h"

// The maximum length of a join or leave message (before substitutions)
#define MAX_FILE_SIZE 1000

struct joins_data
{
	enum auth_credentials min_notify;
	char* join_message;
	char* leave_message;
};

static void set_error_message(struct plugin_handle* plugin, const char* msg)
{
	plugin->error_msg = msg;
}

static char* read_file(const char* filename)
{
	char buf[MAX_FILE_SIZE];
	int fd = open(filename, O_RDONLY);
	int ret;

	if (fd == -1)
		return NULL;

	buf[0] = '\0';

	ret = read(fd, buf, MAX_FILE_SIZE);
	close(fd);

	if (ret > 0)
		buf[ret] = '\0';

	return hub_strdup(buf);
}

static struct joins_data* parse_config(const char* line, struct plugin_handle* plugin)
{
	struct joins_data* data = (struct joins_data*) hub_malloc_zero(sizeof(struct joins_data));
	struct cfg_tokens* tokens = cfg_tokenize(line);
	char* token = cfg_token_get_first(tokens);

	if (!data)
		return NULL;

	data->min_notify = auth_cred_guest;

	while (token)
	{
		struct cfg_settings* setting = cfg_settings_split(token);

		if (!setting)
		{
			set_error_message(plugin, "Unable to parse startup parameters");
			cfg_tokens_free(tokens);
			hub_free(data->join_message);
			hub_free(data->leave_message);
			hub_free(data);
			return NULL;
		}

		if (strcmp(cfg_settings_get_key(setting), "join") == 0)
		{
			if (data->join_message)
			{
				LOG_WARN("Multiple join messages specified, deleting the existing message");
				hub_free(data->join_message);
			}

			data->join_message = read_file(cfg_settings_get_value(setting));
			if (!data->join_message)
			{
				cfg_tokens_free(tokens);
				cfg_settings_free(setting);
				hub_free(data->join_message);
				hub_free(data->leave_message);
				hub_free(data);
				set_error_message(plugin, "Unable to open join message file");
				return NULL;
			}
		}
		else if (strcmp(cfg_settings_get_key(setting), "leave") == 0)
		{
			if (data->leave_message)
			{
				LOG_WARN("Multiple leave messages specified, deleting the existing message");
				hub_free(data->leave_message);
			}

			data->leave_message = read_file(cfg_settings_get_value(setting));
			if (!data->leave_message)
			{
				cfg_tokens_free(tokens);
				cfg_settings_free(setting);
				hub_free(data->join_message);
				hub_free(data->leave_message);
				hub_free(data);
				set_error_message(plugin, "Unable to open leave message file");
				return NULL;
			}
		}
		else if (strcmp(cfg_settings_get_key(setting), "min_notify") == 0)
		{
			auth_string_to_cred(cfg_settings_get_value(setting), &data->min_notify);
		}
		else
		{
			set_error_message(plugin, "Unknown startup parameters given");
			cfg_tokens_free(tokens);
			cfg_settings_free(setting);
			hub_free(data->join_message);
			hub_free(data->leave_message);
			hub_free(data);
			return NULL;
		}

		cfg_settings_free(setting);
		token = cfg_token_get_next(tokens);
	}

	cfg_tokens_free(tokens);
	return data;
}

static struct cbuffer* parse_message(struct plugin_handle* plugin, struct plugin_user* user, const char* msg, const char* reason)
{
	struct joins_data* data = (struct joins_data*) plugin->ptr;
	struct cbuffer* buf = cbuf_create(strlen(msg) + 32);
	const char* start = msg;
	const char* offset = NULL;
	time_t timestamp = time(NULL);
	struct tm now;
	localtime_r(&timestamp, &now);

	while ((offset = strchr(start, '%')))
	{
		cbuf_append_bytes(buf, start, (offset - start));

		offset++;
		// same as mod_welcome
		switch (offset[0])
		{
			case 'n':
				cbuf_append(buf, user->nick);
				break;

			case 'a':
				cbuf_append(buf, plugin->hub.ip_to_string(plugin, &user->addr));
				break;

			case 'c':
				cbuf_append(buf, auth_cred_to_string(user->credentials));
				break;

			case '%':
				cbuf_append(buf, "%");
				break;

			case 'H':
				cbuf_append_strftime(buf, "%H", &now);
				break;

			case 'I':
				cbuf_append_strftime(buf, "%I", &now);
				break;

			case 'M':
				cbuf_append_strftime(buf, "%M", &now);
				break;

			case 'S':
				cbuf_append_strftime(buf, "%S", &now);
				break;

			// NOTE: '%P' here is "AM/PM" which is strftime('%p')
			case 'P':
				cbuf_append_strftime(buf, "%p", &now);
				break;

			// NOTE: '%p' here is "am/pm" which is strftime('%P')
			case 'p':
				cbuf_append_strftime(buf, "%P", &now);
				break;

			case 'Z':
				cbuf_append_strftime(buf, "%Z", &now);
				break;

			case 'z':
				cbuf_append_strftime(buf, "%z", &now);
				break;

			case 'r':
				cbuf_append(buf, reason);
				break;
		}

		start = offset + 1;
	}

	if (*start)
		cbuf_append(buf, start);
	
	cbuf_chomp(buf, NULL);

	return buf;
}

static void user_login(struct plugin_handle* plugin, struct plugin_user* user)
{
	struct joins_data* data = (struct joins_data*) plugin->ptr;
	struct cbuffer* buf = NULL;

	buf = parse_message(plugin, user, data->join_message, "join");
	plugin->hub.send_chat(plugin, data->min_notify, auth_cred_admin, cbuf_get(buf));
	cbuf_destroy(buf);
}

static void user_logout(struct plugin_handle* plugin, struct plugin_user* user, const char* reason)
{
	struct joins_data* data = (struct joins_data*) plugin->ptr;
	struct cbuffer* buf = NULL;

	buf = parse_message(plugin, user, data->leave_message, reason);
	plugin->hub.send_chat(plugin, data->min_notify, auth_cred_admin, cbuf_get(buf));
	cbuf_destroy(buf);
}

int plugin_register(struct plugin_handle* plugin, const char* config)
{
	struct joins_data* data;
	PLUGIN_INITIALIZE(plugin, "Joins plugin", "1.0", "Announces which users join and leave to global chat.");

	data = parse_config(config, plugin);
	plugin->ptr = data;

	if (!data)
		return -1;

	if (!data->join_message && !data->leave_message)
	{
		LOG_WARN("mod_joins loaded without a join or leave message");
		return 0;
	}

	if (data->join_message)
		plugin->funcs.on_user_login = user_login;

	if (data->leave_message)
		plugin->funcs.on_user_logout = user_logout;
	
	return 0;
}

int plugin_unregister(struct plugin_handle* plugin)
{
	struct joins_data* data = (struct joins_data*) plugin->ptr;
	set_error_message(plugin, 0);

	if (data)
	{
		hub_free(data->join_message);
		hub_free(data->leave_message);
	}
    
	hub_free(data);
	return 0;
}
