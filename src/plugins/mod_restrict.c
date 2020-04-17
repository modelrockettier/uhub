/*
 * uhub - A tiny ADC p2p connection hub
 * Copyright (C) 2007-2014, Jan Vidar Krey
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
#include "util/cbuffer.h"
#include "util/config_token.h"
#include "util/log.h"
#include "util/memory.h"

enum chat_warnings
{
	WARN_NONE         =  0x00, ///<<< "No warnings"
	WARN_SEARCH       =  0x01, ///<<< "Warn about searching"
	WARN_CONNECT      =  0x02, ///<<< "Warn about connecting to a user"
	WARN_CHAT         =  0x10, ///<<< "Warn about chat disallowed."
	WARN_PM           =  0x20, ///<<< "Warn about private chat disallowed."
	WARN_OP_PM        =  0x40, ///<<< "Warn about op contact disallowed."
	ALLOW_REVCONNECT  = 0x100, ///<<< "Allow passive user to connect to other user"
	ALLOW_OP_REPLY    = 0x200, ///<<< "Allow users to reply to op PMs."
};

struct user_info
{
	enum chat_warnings warnings; // The number of warnings (used to track whether or not a warning should be sent).
};

struct restrict_data
{
	size_t max_users;        // max users (hard limit max 1M users due to limitations in the SID (20 bits)).
	struct user_info* users; // array of max_users

	enum auth_credentials min_download; // minimum allowed to search and download files.
	enum auth_credentials min_chat;     // minimum allowed to send main chat messages
	enum auth_credentials min_opmsg;    // minimum allowed to send private messages to operators and admins
	enum auth_credentials min_privmsg;  // minimum allowed to send private messages
};

static void set_error_message(struct plugin_handle* plugin, const char* msg)
{
	plugin->error_msg = msg;
}

static struct restrict_data* parse_config(struct plugin_handle* plugin, const char* config)
{
	enum auth_credentials *cred;
	struct restrict_data* data = (struct restrict_data*) hub_malloc_zero(sizeof(struct restrict_data));
	struct cfg_tokens* tokens = cfg_tokenize(config);
	char* token = cfg_token_get_first(tokens);

	if (!data)
	{
		set_error_message(plugin, "OOM");
		cfg_tokens_free(tokens);
		return 0;
	}

	// defaults
	data->max_users = 128;
	data->users = hub_calloc(data->max_users, sizeof(struct user_info));
	data->min_download = auth_cred_guest;
	data->min_chat = auth_cred_guest;
	data->min_opmsg = auth_cred_guest;
	data->min_privmsg = auth_cred_guest;

	if (!data->users)
	{
		set_error_message(plugin, "OOM");
		cfg_tokens_free(tokens);
		hub_free(data);
		return 0;
	}

	while (token)
	{
		struct cfg_settings* setting = cfg_settings_split(token);
		if (!setting)
		{
			set_error_message(plugin, "Unable to parse startup parameters");
			cfg_tokens_free(tokens);
			hub_free(data->users);
			hub_free(data);
			return 0;
		}

		cred = NULL;
		if (strcmp(cfg_settings_get_key(setting), "download") == 0)
		{
			cred = &data->min_download;
		}
		else if (strcmp(cfg_settings_get_key(setting), "priv_msg") == 0)
		{
			cred = &data->min_privmsg;
		}
		else if (strcmp(cfg_settings_get_key(setting), "chat") == 0)
		{
			cred = &data->min_chat;
		}
		else if (strcmp(cfg_settings_get_key(setting), "op_msg") == 0)
		{
			cred = &data->min_opmsg;
		}
		else
		{
			set_error_message(plugin, "Unknown startup parameter given");
			cfg_settings_free(setting);
			cfg_tokens_free(tokens);
			hub_free(data->users);
			hub_free(data);
			return 0;
		}

		if (!auth_string_to_cred(cfg_settings_get_value(setting), cred))
		{
			set_error_message(plugin, "Unknown credential value");
			cfg_settings_free(setting);
			cfg_tokens_free(tokens);
			hub_free(data->users);
			hub_free(data);
			return 0;
		}

		cfg_settings_free(setting);
		token = cfg_token_get_next(tokens);
	}
	cfg_tokens_free(tokens);

	// Normally operators should be allowed to send messages
	if (data->min_privmsg > auth_cred_operator)
		LOG_WARN("Operators aren't allowed to send private messages");
	if (data->min_opmsg > auth_cred_operator)
		LOG_WARN("Operators aren't allowed to message each other");

	return data;
}

static void restrict_shutdown(struct plugin_handle* plugin)
{
	struct restrict_data* data = (struct restrict_data*) plugin->ptr;
	if (data)
	{
		hub_free(data->users);
		hub_free(data);
		plugin->ptr = NULL;
	}
}

static struct user_info* get_user_info(struct restrict_data* data, sid_t sid)
{
	struct user_info* u;

	// resize buffer if needed.
	if (sid >= data->max_users)
	{
		// allocate 32 extra user slots so we don't have to do this every time
		// a new user joins
		size_t n = ((size_t) sid + 32) & ~((size_t) 32);
		u = hub_realloc(data->users, n * sizeof(struct user_info));
		if (!u)
		{
			LOG_FATAL("mod_restrict: No memory for " PRINTF_SIZE_T " users", n);
			return NULL;
		}
		// zero out the new user structs
		memset(&u[data->max_users], 0, (n - data->max_users) * sizeof(struct user_info));

		data->users = u;
		data->max_users = n;
	}

	u = &data->users[sid];

	return u;
}

static void send_warning(struct plugin_handle* plugin, struct plugin_user* user, enum chat_warnings warning)
{
	struct restrict_data* data = (struct restrict_data*) plugin->ptr;
	struct cbuffer* buf = NULL;
	struct user_info* info = get_user_info(data, user->sid);

	warning &= ~info->warnings;

	// User has already been warned about this
	if (warning == WARN_NONE)
		return;

	buf = cbuf_create(128);
	cbuf_append(buf, "You are not allowed to ");

	if (warning == WARN_CHAT)
		cbuf_append(buf, "send chat messages");
	else if (warning == WARN_OP_PM)
		cbuf_append(buf, "contact operators or admins");
	else if (warning == WARN_PM)
		cbuf_append(buf, "send private messages");
	else if (warning == WARN_SEARCH)
		cbuf_append(buf, "search");
	else if (warning == WARN_CONNECT)
		cbuf_append(buf, "connect to other users");
	else
	{
		LOG_ERROR("Unknown warning value: %#x", warning);
		cbuf_destroy(buf);
		return;
	}

	cbuf_append(buf, " on this hub.");

	plugin->hub.send_message(plugin, user, cbuf_get(buf));
	cbuf_destroy(buf);

	info->warnings |= warning;
}

static plugin_st on_search_result(struct plugin_handle* plugin, struct plugin_user* from, struct plugin_user* to, const char* search_data)
{
	struct restrict_data* data = (struct restrict_data*) plugin->ptr;

	if (to->credentials >= data->min_download)
		return st_default;

	// TODO: warn?
	return st_deny;
}

static plugin_st on_search(struct plugin_handle* plugin, struct plugin_user* user, const char* search_data)
{
	struct restrict_data* data = (struct restrict_data*) plugin->ptr;

	if (user->credentials >= data->min_download)
		return st_default;

	send_warning(plugin, user, WARN_SEARCH);
	return st_deny;
}

static plugin_st on_p2p_connect(struct plugin_handle* plugin, struct plugin_user* from, struct plugin_user* to)
{
	struct restrict_data* data = (struct restrict_data*) plugin->ptr;

	if (from->credentials >= data->min_download)
		return st_default;

	struct user_info* target = get_user_info(data, to->sid);
	if (target->warnings & ALLOW_REVCONNECT)
		return st_default;

	send_warning(plugin, from, WARN_CONNECT);
	return st_deny;
}

static plugin_st on_p2p_revconnect(struct plugin_handle* plugin, struct plugin_user* from, struct plugin_user* to)
{
	struct restrict_data* data = (struct restrict_data*) plugin->ptr;

	if (from->credentials >= data->min_download)
	{
		struct user_info* info = get_user_info(data, from->sid);
		info->warnings |= ALLOW_REVCONNECT;
		return st_default;
	}

	send_warning(plugin, from, WARN_CONNECT);
	return st_deny;
}

plugin_st on_chat_msg(struct plugin_handle* plugin, struct plugin_user* from, const char* message)
{
	struct restrict_data* data = (struct restrict_data*) plugin->ptr;

	if (from->credentials >= data->min_chat)
		return st_default;

	send_warning(plugin, from, WARN_CHAT);
	return st_deny;
}

plugin_st on_private_msg(struct plugin_handle* plugin, struct plugin_user* from, struct plugin_user* to, const char* message)
{
	struct restrict_data* data = (struct restrict_data*) plugin->ptr;

	// recipient is op
	if (to->credentials >= auth_cred_operator)
	{
		// sender is allowed to contact ops
		if (from->credentials >= data->min_opmsg)
			return st_default;

		struct user_info* info = get_user_info(data, from->sid);
		// sender was contacted by op in the past, allow replies
		if (info->warnings & ALLOW_OP_REPLY)
			return st_default;

		send_warning(plugin, from, WARN_OP_PM);
		return st_deny;
	}

	if (from->credentials < data->min_privmsg)
	{
		send_warning(plugin, from, WARN_PM);
		return st_deny;
	}

	// allow target to respond to operator PMs
	if (from->credentials >= auth_cred_operator && to->credentials < data->min_opmsg)
	{
		struct user_info* target = get_user_info(data, to->sid);
		target->warnings |= ALLOW_OP_REPLY;
	}

	return st_default;
}

static void on_user_login(struct plugin_handle* plugin, struct plugin_user* user)
{
	struct restrict_data* data = (struct restrict_data*) plugin->ptr;
	struct user_info* info = get_user_info(data, user->sid);

	info->warnings = WARN_NONE;
}

static void on_user_logout(struct plugin_handle* plugin, struct plugin_user* user, const char* reason)
{
	struct restrict_data* data = (struct restrict_data*) plugin->ptr;
	struct user_info* info = get_user_info(data, user->sid);

	memset(info, 0, sizeof(*info));
}

int plugin_register(struct plugin_handle* plugin, const char* config)
{
	struct restrict_data* data;
	int restrictions = 0;
	PLUGIN_INITIALIZE(plugin, "Restricted hub", "1.0", "Restrict who can send chat messages, private messages, contact operators, search, or download files.");

	data = parse_config(plugin, config);
	if (!data)
		return -1;

	plugin->ptr = data;
	if (data->min_download > auth_cred_guest)
	{
		restrictions = 1;
		plugin->funcs.on_search = on_search;
		plugin->funcs.on_search_result = on_search_result;
		plugin->funcs.on_p2p_connect = on_p2p_connect;
		plugin->funcs.on_p2p_revconnect = on_p2p_revconnect;
	}
	if (data->min_chat > auth_cred_guest)
	{
		restrictions = 1;
		plugin->funcs.on_chat_msg = on_chat_msg;
	}
	if (data->min_privmsg > auth_cred_guest || data->min_opmsg > auth_cred_guest)
	{
		restrictions = 1;
		plugin->funcs.on_private_msg = on_private_msg;
	}

	if (restrictions)
	{
		plugin->funcs.on_user_login = on_user_login;
		plugin->funcs.on_user_logout = on_user_logout;
	}
	else
	{
		/* No restrictions */
		restrict_shutdown(plugin);
	}

	return 0;
}

int plugin_unregister(struct plugin_handle* plugin)
{
	restrict_shutdown(plugin);
	return 0;
}

