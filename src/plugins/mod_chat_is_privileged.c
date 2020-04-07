/*
 * uhub - A tiny ADC p2p connection hub
 * Copyright (C) 2007-2014, Jan Vidar Krey
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
#include "util/memory.h"

enum chat_warnings {
	warn_none = 0,
	warn_mainchat = 1,
	warn_privchat = 2,
	warn_opcontact = 4,
};

struct user_info
{
	sid_t sid;
	enum chat_warnings warnings;
};

struct chat_restrictions_data
{
	size_t num_users;        // number of users tracked.
	size_t max_users;        // max users (hard limit max 1M users due to limitations in the SID (20 bits)).
	struct user_info* users; // array of max_users

	enum auth_credentials allow_privchat;   // minimum credentials to allow using private chat
	enum auth_credentials allow_op_contact; // minimum credentials to allow private chat to operators (including super and admins).
	enum auth_credentials allow_mainchat;   // minimum credentials to allow using main chat
};

static void set_error_message(struct plugin_handle* plugin, const char* msg)
{
	plugin->error_msg = msg;
}

static struct chat_restrictions_data* parse_config(struct plugin_handle* plugin, const char* config)
{
	enum auth_credentials *cred;
	struct chat_restrictions_data* data = (struct chat_restrictions_data*) hub_malloc(sizeof(struct chat_restrictions_data));
	struct cfg_tokens* tokens = cfg_tokenize(config);
	char* token = cfg_token_get_first(tokens);

	// defaults
	data->num_users = 0;
	data->max_users = 512;
	data->users = hub_malloc_zero(sizeof(struct user_info) * data->max_users);
	data->allow_mainchat = auth_cred_guest;
	data->allow_op_contact = auth_cred_guest;
	data->allow_privchat = auth_cred_guest;

	while (token)
	{
		struct cfg_settings* setting = cfg_settings_split(token);

		if (!setting)
		{
			set_error_message(plugin, "Unable to parse startup parameters");
			cfg_tokens_free(tokens);
			hub_free(data);
			return 0;
		}

		cred = NULL;

		if (strcmp(cfg_settings_get_key(setting), "priv_chat") == 0)
		{
			cred = &data->allow_privchat;
		}
		else if (strcmp(cfg_settings_get_key(setting), "main_chat") == 0)
		{
			cred = &data->allow_mainchat;
		}
		else if (strcmp(cfg_settings_get_key(setting), "op_contact") == 0)
		{
			cred = &data->allow_op_contact;
		}
		else
		{
			set_error_message(plugin, "Unknown startup parameters given");
			cfg_tokens_free(tokens);
			cfg_settings_free(setting);
			hub_free(data);
			return 0;
		}

		if (!auth_string_to_cred(cfg_settings_get_value(setting), cred))
		{
			set_error_message(plugin, "Unknown credential value");
			cfg_tokens_free(tokens);
			hub_free(data);
			return 0;
		}

		cfg_settings_free(setting);
		token = cfg_token_get_next(tokens);
	}
	cfg_tokens_free(tokens);

	return data;
}

static struct user_info* get_user_info(struct chat_restrictions_data* data, sid_t sid)
{
	struct user_info* u;

	// resize buffer if needed.
	if (sid >= data->max_users)
	{
		u = hub_malloc_zero(sizeof(struct user_info) * (sid + 1));
		memcpy(u, data->users, data->max_users);
		hub_free(data->users);
		data->users = u;
		data->max_users = sid;
		u = NULL;
	}

	u = &data->users[sid];

	// reset counters if the user was not previously known.
	if (!u->sid)
	{
		u->sid = sid;
		u->warnings = warn_none;
		data->num_users++;
	}
	return u;
}

static void send_warning(struct plugin_handle* plugin, struct plugin_user* user, enum chat_warnings warning)
{
	struct chat_restrictions_data* data = (struct chat_restrictions_data*) plugin->ptr;
	struct cbuffer* buf = NULL;
	struct user_info* info = get_user_info(data, user->sid);

	warning &= ~info->warnings;
	if (!warning)
		return;

	buf = cbuf_create(128);
	if (warning & warn_mainchat)
		cbuf_append(buf, "You are not allowed to chat.");
	else if (warning & warn_opcontact)
		cbuf_append(buf, "You are not allowed to contact operators or admins.");
	else if (warning & warn_privchat)
		cbuf_append(buf, "You are not allowed to send private messages.");

	plugin->hub.send_message(plugin, user, cbuf_get(buf));
	cbuf_destroy(buf);

	info->warnings |= warning;
}

static void on_user_login(struct plugin_handle* plugin, struct plugin_user* user)
{
	struct chat_restrictions_data* data = (struct chat_restrictions_data*) plugin->ptr;
	/*struct user_info* info = */
	get_user_info(data, user->sid);
}

static void on_user_logout(struct plugin_handle* plugin, struct plugin_user* user, const char* reason)
{
	struct chat_restrictions_data* data = (struct chat_restrictions_data*) plugin->ptr;
	struct user_info* info = get_user_info(data, user->sid);
	if (info->sid)
		data->num_users--;
	info->warnings = warn_none;
	info->sid = 0;
}

plugin_st on_chat_msg(struct plugin_handle* plugin, struct plugin_user* from, const char* message)
{
	struct chat_restrictions_data* data = (struct chat_restrictions_data*) plugin->ptr;

	if (from->credentials >= data->allow_mainchat)
		return st_default;

	send_warning(plugin, from, warn_mainchat);
	return st_deny;
}

plugin_st on_private_msg(struct plugin_handle* plugin, struct plugin_user* from, struct plugin_user* to, const char* message)
{
	struct chat_restrictions_data* data = (struct chat_restrictions_data*) plugin->ptr;
	if (to->credentials >= auth_cred_operator)
	{
		if (from->credentials >= data->allow_op_contact)
			return st_default;

		send_warning(plugin, from, warn_opcontact);
		return st_deny;
	}

	if (from->credentials >= data->allow_privchat)
		return st_default;

	send_warning(plugin, from, warn_privchat);
	return st_deny;
}


int plugin_register(struct plugin_handle* plugin, const char* config)
{
	struct chat_restrictions_data* data;
	PLUGIN_INITIALIZE(plugin, "Privileged chat hub", "1.0", "Restrict who can send chat messages, private messages, or contact operators.");

	data = parse_config(plugin, config);
	plugin->ptr = data;

	if (!data)
		return -1;

	plugin->funcs.on_user_login = on_user_login;
	plugin->funcs.on_user_logout = on_user_logout;
	plugin->funcs.on_chat_msg = on_chat_msg;
	plugin->funcs.on_private_msg = on_private_msg;

	if (data->allow_mainchat > auth_cred_guest)
		return 0;
	else if (data->allow_op_contact > auth_cred_guest)
		return 0;
	else if (data->allow_privchat > auth_cred_guest)
		return 0;

	// guests are allowed in all chats, no use enabling this plugin
	hub_free(data->users);
	hub_free(data);
	return -1;
}

int plugin_unregister(struct plugin_handle* plugin)
{
	struct chat_restrictions_data* data = (struct chat_restrictions_data*) plugin->ptr;
	if (data)
	{
		hub_free(data->users);
		hub_free(data);
	}
	return 0;
}

