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
#include "plugin_api/command_api.h"
#include <sqlite3.h>
#include "util/memory.h"
#include "util/list.h"
#include "util/misc.h"
#include "util/log.h"
#include "util/config_token.h"
#include "util/cbuffer.h"

static void set_error_message(struct plugin_handle* plugin, const char* msg)
{
	plugin->error_msg = msg;
}

struct user_manager
{
	int register_self; ///<<< "Enable self-registration"
	int allow_spaces; ///<<< "Allow users to register with spaces in their name (hard to manage)"
	int notify; ///<<< "Notify ops about attempts to self-register and notify admins about failed permission checks"
	size_t password_length; ///<<< "The minimum password length"

	struct plugin_command_handle* cmd_password; ///<<< "A handle to the !password command."
	struct plugin_command_handle* cmd_register; ///<<< "A handle to the !register command."
	struct plugin_command_handle* cmd_useradd;  ///<<< "A handle to the !useradd command."
	struct plugin_command_handle* cmd_userdel;  ///<<< "A handle to the !userdel command."
	struct plugin_command_handle* cmd_userlist; ///<<< "A handle to the !userlist command."
	struct plugin_command_handle* cmd_usermod;  ///<<< "A handle to the !usermod command."
	struct plugin_command_handle* cmd_userpass; ///<<< "A handle to the !userpass command."
};

static struct user_manager* parse_config(const char* line, struct plugin_handle* plugin)
{
	struct user_manager* manager = (struct user_manager*) hub_malloc_zero(sizeof(struct user_manager));
	struct cfg_tokens* tokens = cfg_tokenize(line);
	char* token = cfg_token_get_first(tokens);

	if (!manager)
	{
		set_error_message(plugin, "OOM");
		cfg_tokens_free(tokens);
		return 0;
	}

	manager->register_self = 0;
	manager->allow_spaces = 0;
	manager->notify = 1;
	manager->password_length = 6;

	while (token)
	{
		struct cfg_settings* setting = cfg_settings_split(token);

		if (!setting)
		{
			set_error_message(plugin, "Unable to parse startup parameters");
			cfg_tokens_free(tokens);
			hub_free(manager);
			return 0;
		}

		if (strcmp(cfg_settings_get_key(setting), "register_self") == 0)
		{
			if (!string_to_boolean(cfg_settings_get_value(setting), &manager->register_self))
				manager->register_self = 1;
		}
		else if (strcmp(cfg_settings_get_key(setting), "allow_spaces") == 0)
		{
			if (!string_to_boolean(cfg_settings_get_value(setting), &manager->allow_spaces))
				manager->allow_spaces = 1;
		}
		else if (strcmp(cfg_settings_get_key(setting), "notify") == 0)
		{
			if (!string_to_boolean(cfg_settings_get_value(setting), &manager->notify))
				manager->notify = 1;
		}
		else if (strcmp(cfg_settings_get_key(setting), "password_length") == 0)
		{
			manager->password_length = (size_t) uhub_atoi(cfg_settings_get_value(setting));
			if (manager->password_length > MAX_PASS_LEN)
			{
				LOG_WARN("Minimum password length (" PRINTF_SIZE_T ") exceeds "
					"the maximum (%d)", manager->password_length, MAX_PASS_LEN);

				manager->password_length = MAX_PASS_LEN;
			}
		}
		else
		{
			set_error_message(plugin, "Unknown startup parameters given");
			cfg_tokens_free(tokens);
			cfg_settings_free(setting);
			hub_free(manager);
			return 0;
		}

		cfg_settings_free(setting);
		token = cfg_token_get_next(tokens);
	}
	cfg_tokens_free(tokens);

	return manager;
}

static int copy_nickname(struct plugin_handle* plugin, struct auth_info* userinfo,
	char const* nickname, char* error, size_t errlen)
{
	size_t nickname_length = strlen(nickname);

	if (nickname_length <= 0 || MAX_NICK_LEN < nickname_length)
	{
		snprintf(error, errlen, "Nickname must be between 1 and %d characters long.",
			MAX_NICK_LEN);

		return 0;
	}

	strlcpy(userinfo->nickname, nickname, MAX_NICK_LEN+1);

	return 1;
}

static int copy_password(struct plugin_handle* plugin, struct auth_info* userinfo,
	char const* password, char* error, size_t errlen)
{
	struct user_manager* manager = (struct user_manager*) plugin->ptr;

	// special "none" password
	if (!strcmp(password, "none"))
		password = "";

	size_t password_length = strlen(password);

	if (password_length < manager->password_length || password_length > MAX_PASS_LEN)
	{
		snprintf(error, errlen, "Password must be between %d and %d characters long.",
			(int) manager->password_length, MAX_PASS_LEN);

		return 0;
	}

	strlcpy(userinfo->password, password, MAX_PASS_LEN+1);

	return 1;
}

// NOTE: userinfo->nickname must be set
static int copy_credentials(struct plugin_handle* plugin, struct plugin_user* user, struct auth_info* userinfo,
	enum auth_credentials credentials, int new, char* error, size_t errlen)
{
	// no guest users
	if (credentials < auth_cred_user)
	{
		const char* cred_str = auth_cred_to_string(credentials);
		if (new)
			snprintf(error, errlen, "\"%s\" users can't be registered.", cred_str);
		else
			snprintf(error, errlen, "Can't demote a user to \"%s\".", cred_str);
	}
	// prevents promoting yourself to an admin via a second account
	else if (credentials > user->credentials)
	{
		const char* cred_str = auth_cred_to_string(credentials);
		const char* user_cred_str = auth_cred_to_string(user->credentials);
		if (new)
			snprintf(error, errlen, "You can't add a \"%s\" user as a \"%s\".", cred_str, user_cred_str);
		else if (!strcmp(userinfo->nickname, user->nick))
			snprintf(error, errlen, "You can't promote yourself.");
		else
			snprintf(error, errlen, "You can't promote a user to \"%s\" as a \"%s\".", cred_str, user_cred_str);

		struct user_manager* manager = (struct user_manager*) plugin->ptr;
		if (manager->notify)
		{
			struct cbuffer* buf = cbuf_create(128);
			if (new)
				cbuf_append_format(buf, "User \"%s\" tried to create a new %s.", user->nick, cred_str);
			else if (!strcmp(userinfo->nickname, user->nick))
				cbuf_append_format(buf, "User \"%s\" tried to promote themself to a %s.", user->nick, cred_str);
			else
				cbuf_append_format(buf, "User \"%s\" tried to promote a user to %s.", user->nick, cred_str);

			plugin->hub.send_chat(plugin, user->credentials+1, auth_cred_admin, cbuf_get(buf));
			cbuf_destroy(buf);
		}
	}
	else
	{
		userinfo->credentials = credentials;
		return 1;
	}

	return 0;
}

// Returns 0 if the nick has spaces and spaces aren't allowed, 1 otherwise
static int nick_space_check(struct plugin_handle* plugin, char const* nick)
{
	struct user_manager* manager = (struct user_manager*) plugin->ptr;

	// spaces are allowed, we're good
	if (manager->allow_spaces)
		return 1;

	// check for spaces
	while (*nick)
	{
		if (is_white_space(*nick))
			return 0;

		nick++;
	}

	return 1;
}

static int nick_is_registered(struct plugin_handle* plugin, const char* nick)
{
	struct auth_info tmp;
	if (plugin->hub.auth_get_user(plugin, nick, &tmp) != st_allow)
		return 0;

	// workaround for the guest-passwd plugin showing everyone as registered
	return (tmp.credentials >= auth_cred_user) ? 1 : 0;
}

// find a user to modify, returns 0 if the user wasn't found or has higher privileges
static int find_user(struct plugin_handle* plugin, struct plugin_user* user,
	const char* nick, const char* action, struct auth_info* userinfo, char* error, size_t errlen)
{
	if (plugin->hub.auth_get_user(plugin, nick, userinfo) != st_allow)
		snprintf(error, errlen, "Unable to find user \"%s\".", nick);
	// workaround for the guest-passwd plugin showing everyone as registered
	else if (userinfo->credentials < auth_cred_user)
		snprintf(error, errlen, "User \"%s\" is not registered.", nick);
	else if (userinfo->credentials > user->credentials)
	{
		struct user_manager* manager = (struct user_manager*) plugin->ptr;
		if (manager->notify)
		{
			struct cbuffer* buf = cbuf_create(128);
			cbuf_append_format(buf, "User \"%s\" tried to %s %s \"%s\".", user->nick,
				action, auth_cred_to_string(userinfo->credentials), userinfo->nickname);

			plugin->hub.send_chat(plugin, user->credentials+1, auth_cred_admin, cbuf_get(buf));
			cbuf_destroy(buf);
		}

		snprintf(error, errlen, "Insufficient rights.");
	}
	else
		return 1;

	return 0;
}

static int command_register(struct plugin_handle* plugin, struct plugin_user* user, struct plugin_command* cmd)
{
	struct user_manager* manager = (struct user_manager*) plugin->ptr;
	struct cbuffer* buf = cbuf_create(128);
	struct auth_info userinfo;
	struct plugin_command_arg_data* arg1 = plugin->hub.command_arg_next(plugin, cmd, plugin_cmd_arg_type_string);
	char* password = arg1->data.string;
	char error[128];

	memset(&userinfo, 0, sizeof(userinfo));

	userinfo.credentials = auth_cred_user;
	// the user's nick has already been validated
	strlcpy(userinfo.nickname, user->nick, MAX_NICK_LEN+1);
	// userinfo.password will be validated and set by copy_password()

	if (!copy_password(plugin, &userinfo, password, error, sizeof(error)))
		cbuf_append_format(buf, "*** %s: %s", cmd->prefix, error);
	else if (!nick_space_check(plugin, user->nick))
		cbuf_append_format(buf, "*** %s: You are not allowed to register with spaces in your name.", cmd->prefix);
	else
	{
		if (nick_is_registered(plugin, user->nick))
			cbuf_append_format(buf, "*** %s: User \"%s\" is already registered.", cmd->prefix, user->nick);
		else if (plugin->hub.auth_register_user(plugin, &userinfo) != st_allow)
			cbuf_append_format(buf, "*** %s: Unable to register user \"%s\".", cmd->prefix, user->nick);
		else
		{
			cbuf_append_format(buf, "*** %s: User \"%s\" registered.", cmd->prefix, user->nick);
			// update user's credentials now that they're registered
			user->credentials = userinfo.credentials;
		}

		if (manager->notify)
			plugin->hub.send_chat(plugin, auth_cred_operator, auth_cred_admin, cbuf_get(buf));
	}

	plugin->hub.send_message(plugin, user, cbuf_get(buf));
	cbuf_destroy(buf);
	return 0;
}

static int command_password(struct plugin_handle* plugin, struct plugin_user* user, struct plugin_command* cmd)
{
	struct cbuffer* buf = cbuf_create(128);
	struct auth_info userinfo;
	struct plugin_command_arg_data* arg1 = plugin->hub.command_arg_next(plugin, cmd, plugin_cmd_arg_type_string);
	char* password = arg1->data.string;
	char error[128];

	// the user's nick and creds have already been validated
	strlcpy(userinfo.nickname, user->nick, MAX_NICK_LEN+1);
	userinfo.credentials = user->credentials;
	// userinfo.password will be validated and set by copy_password()

	if (user->credentials < auth_cred_user)
		cbuf_append_format(buf, "*** %s: You are not a registered user.", cmd->prefix);
	else if (!copy_password(plugin, &userinfo, password, error, sizeof(error)))
		cbuf_append_format(buf, "*** %s: %s", cmd->prefix, error);
	else if (plugin->hub.auth_update_user(plugin, &userinfo) != st_allow)
		cbuf_append_format(buf, "*** %s: Unable to change password.", cmd->prefix);
	else
		cbuf_append_format(buf, "*** %s: Password changed.", cmd->prefix);

	plugin->hub.send_message(plugin, user, cbuf_get(buf));
	cbuf_destroy(buf);

	return 0;
}

static int command_useradd(struct plugin_handle* plugin, struct plugin_user* user, struct plugin_command* cmd)
{
	struct cbuffer* buf = cbuf_create(128);
	struct auth_info userinfo;
	struct plugin_command_arg_data* arg1 = plugin->hub.command_arg_next(plugin, cmd, plugin_cmd_arg_type_string);
	struct plugin_command_arg_data* arg2 = plugin->hub.command_arg_next(plugin, cmd, plugin_cmd_arg_type_string);
	struct plugin_command_arg_data* arg3 = plugin->hub.command_arg_next(plugin, cmd, plugin_cmd_arg_type_credentials);
	char error[128];

	char* nick = arg1->data.string;
	char* password = arg2->data.string;
	// use credentials if they were specified, otherwise they're a regular user
	enum auth_credentials credentials = (arg3) ? arg3->data.credentials : auth_cred_user;

	// userinfo.nickname    will be validated and set by copy_nickname()
	// userinfo.password    will be validated and set by copy_password()
	// userinfo.credentials will be validated and set by copy_credentials()

	if (!copy_nickname(plugin, &userinfo, nick, error, sizeof(error)))
		cbuf_append_format(buf, "*** %s: %s", cmd->prefix, error);
	else if (!copy_password(plugin, &userinfo, password, error, sizeof(error)))
		cbuf_append_format(buf, "*** %s: %s", cmd->prefix, error);
	else if (!copy_credentials(plugin, user, &userinfo, credentials, 1, error, sizeof(error)))
		cbuf_append_format(buf, "*** %s: %s", cmd->prefix, error);
	else if (nick_is_registered(plugin, nick))
		cbuf_append_format(buf, "*** %s: User \"%s\" is already registered.", cmd->prefix, nick);
	else if (plugin->hub.auth_register_user(plugin, &userinfo) != st_allow)
		cbuf_append_format(buf, "*** %s: Unable to register user \"%s\".", cmd->prefix, nick);
	else
	{
		// If the user is connected, let them know their account has been created
		struct plugin_user* target = plugin->hub.get_user_by_nick(plugin, userinfo.nickname);
		if (target)
		{
			cbuf_append_format(buf, "An account has been created for you by \"%s\".", user->nick);

			plugin->hub.send_message(plugin, target, cbuf_get(buf));
			cbuf_clear(buf);
		}

		cbuf_append_format(buf, "*** %s: User \"%s\" registered.", cmd->prefix, nick);
	}

	// won't affect any already logged in guest users (they'll have to reconnect for it to take effect)

	plugin->hub.send_message(plugin, user, cbuf_get(buf));
	cbuf_destroy(buf);

	return 0;
}

static int command_userdel(struct plugin_handle* plugin, struct plugin_user* user, struct plugin_command* cmd)
{
	struct cbuffer* buf = cbuf_create(128);
	struct auth_info userinfo;
	struct plugin_command_arg_data* arg1 = plugin->hub.command_arg_next(plugin, cmd, plugin_cmd_arg_type_string);
	char* nick = arg1->data.string;
	char error[128];

	if (!find_user(plugin, user, nick, "delete", &userinfo, error, sizeof(error)))
		cbuf_append_format(buf, "*** %s: %s", cmd->prefix, error);
	else if (!strcmp(user->nick, nick))
		cbuf_append_format(buf, "*** %s: You can not delete yourself.", cmd->prefix);
	else if (plugin->hub.auth_delete_user(plugin, &userinfo) != st_allow)
		cbuf_append_format(buf, "*** %s: Unable to delete user \"%s\".", cmd->prefix, nick);
	else
	{
		// Set the user to a guest if they're connected
		struct plugin_user* target = plugin->hub.get_user_by_nick(plugin, userinfo.nickname);
		if (target && target->credentials >= auth_cred_user)
		{
			cbuf_append_format(buf, "Your account has been deleted by \"%s\".", user->nick);
			plugin->hub.send_message(plugin, target, cbuf_get(buf));
			cbuf_clear(buf);

			target->credentials = auth_cred_guest;
			//plugin->hub.user_disconnect(target);
		}

		cbuf_append_format(buf, "*** %s: User \"%s\" deleted.", cmd->prefix, nick);
	}

	plugin->hub.send_message(plugin, user, cbuf_get(buf));
	cbuf_destroy(buf);

	return 0;
}

static int command_usermod(struct plugin_handle* plugin, struct plugin_user* user, struct plugin_command* cmd)
{
	struct cbuffer* buf = cbuf_create(128);
	struct auth_info userinfo;
	struct plugin_command_arg_data* arg1 = plugin->hub.command_arg_next(plugin, cmd, plugin_cmd_arg_type_string);
	struct plugin_command_arg_data* arg2 = plugin->hub.command_arg_next(plugin, cmd, plugin_cmd_arg_type_credentials);
	char* nick = arg1->data.string;
	enum auth_credentials credentials = arg2->data.credentials;
	char error[128];

	if (!find_user(plugin, user, nick, "promote/demote", &userinfo, error, sizeof(error)))
		cbuf_append_format(buf, "*** %s: %s", cmd->prefix, error);
	else if (userinfo.credentials == credentials)
	{
		cbuf_append_format(buf, "*** %s: User \"%s\" is already a %s.", cmd->prefix, nick,
			auth_cred_to_string(credentials));
	}
	else
	{
		const char* moted = (userinfo.credentials < credentials) ? "promoted" : "demoted";

		if (!copy_credentials(plugin, user, &userinfo, credentials, 0, error, sizeof(error)))
			cbuf_append_format(buf, "*** %s: %s", cmd->prefix, error);
		else if (!strcmp(user->nick, nick))
			cbuf_append_format(buf, "*** %s: You can not demote yourself.", cmd->prefix);
		else if (plugin->hub.auth_update_user(plugin, &userinfo) != st_allow)
			cbuf_append_format(buf, "*** %s: Unable to change credentials for user \"%s\".", cmd->prefix, nick);
		else {
			const char* cred_str = auth_cred_to_string(userinfo.credentials);

			// update the user's credentials if they're already logged in (connected and not a guest)
			struct plugin_user* target = plugin->hub.get_user_by_nick(plugin, userinfo.nickname);
			if (target && target->credentials >= auth_cred_user)
			{
				target->credentials = userinfo.credentials;

				cbuf_append_format(buf, "You have been %s to a %s by \"%s\".",
					moted, cred_str, user->nick);

				plugin->hub.send_message(plugin, target, cbuf_get(buf));
				cbuf_clear(buf);
			}

			cbuf_append_format(buf, "*** %s: User \"%s\" %s to %s.",
				cmd->prefix, nick, moted, cred_str);
		}
	}

	plugin->hub.send_message(plugin, user, cbuf_get(buf));
	cbuf_destroy(buf);

	return 0;
}

static int command_userpass(struct plugin_handle* plugin, struct plugin_user* user, struct plugin_command* cmd)
{
	struct cbuffer* buf = cbuf_create(128);
	struct auth_info userinfo;
	struct plugin_command_arg_data* arg1 = plugin->hub.command_arg_next(plugin, cmd, plugin_cmd_arg_type_string);
	struct plugin_command_arg_data* arg2 = plugin->hub.command_arg_next(plugin, cmd, plugin_cmd_arg_type_string);
	char* nick = arg1->data.string;
	char* password = arg2->data.string;
	char error[128];

	if (!find_user(plugin, user, nick, "change password for", &userinfo, error, sizeof(error)))
		cbuf_append_format(buf, "*** %s: %s", cmd->prefix, error);
	else if (!copy_password(plugin, &userinfo, password, error, sizeof(error)))
		cbuf_append_format(buf, "*** %s: %s", cmd->prefix, error);
	else if (plugin->hub.auth_update_user(plugin, &userinfo) != st_allow)
		cbuf_append_format(buf, "*** %s: Unable to change password for user \"%s\".", cmd->prefix, nick);
	// op used !userpass to change their own password
	else if (!strcmp(user->nick, userinfo.nickname))
		cbuf_append_format(buf, "*** %s: Your password has been changed.", cmd->prefix);
	else
	{
		// If the user is connected, let them know their password changed
		struct plugin_user* target = plugin->hub.get_user_by_nick(plugin, userinfo.nickname);
		if (target)
		{
			cbuf_append_format(buf, "Your password has been changed by \"%s\".", user->nick);

			plugin->hub.send_message(plugin, target, cbuf_get(buf));
			cbuf_clear(buf);
		}

		cbuf_append_format(buf, "*** %s: Password for user \"%s\" changed.", cmd->prefix, nick);
	}

	plugin->hub.send_message(plugin, user, cbuf_get(buf));
	cbuf_destroy(buf);

	return 0;
}

static int command_userlist(struct plugin_handle* plugin, struct plugin_user* user, struct plugin_command* cmd)
{
	struct cbuffer* buf = cbuf_create(512);
	struct linked_list* users = (struct linked_list*) list_create();
	struct plugin_command_arg_data* arg1 = plugin->hub.command_arg_next(plugin, cmd, plugin_cmd_arg_type_string);
	int rc;

	const char* search = (arg1) ? arg1->data.string : NULL;

	rc = plugin->hub.auth_get_user_list(plugin, search, users);
	if (rc == 0)
		cbuf_append_format(buf, "*** %s: Failed to get user list.", cmd->prefix);
	else if (!list_size(users))
		cbuf_append_format(buf, "*** %s: No users found.", cmd->prefix);
	else
	{
		size_t user_count = list_size(users);
		cbuf_append_format(buf, "*** %s: " PRINTF_SIZE_T " user%s\n",
			cmd->prefix, user_count, user_count != 1 ? "s" : "");

		struct auth_info* list_item = (struct auth_info*) list_get_first(users);

		while (list_item)
		{
			cbuf_append_format(buf, "Credentials: %s, Nickname: %s, Last Login: %s\n",
				auth_cred_to_string(list_item->credentials), list_item->nickname,
				list_item->activity);

			list_item = (struct auth_info*) list_get_next(users);
		}
	}

	plugin->hub.send_message(plugin, user, cbuf_get(buf));
	list_clear(users, &hub_free);
	list_destroy(users);
	cbuf_destroy(buf);

	return 0;
}

// Visual Studio can't handle variadic macros here for some reason
#define CMD_ADD(HANDLE, PLUGIN, PREFIX, ARGS, CRED, CALLBACK, DESC) \
	do { \
		(HANDLE) = (struct plugin_command_handle*) hub_malloc(sizeof(struct plugin_command_handle)); \
		PLUGIN_COMMAND_INITIALIZE((HANDLE), PLUGIN, PREFIX, ARGS, CRED, CALLBACK, DESC); \
		if ((PLUGIN)->hub.command_add(PLUGIN, HANDLE) != 0) \
			LOG_ERROR("Failed to add command: %s", PREFIX); \
	} while (0)

#define CMD_DEL(PLUGIN, HANDLE) \
	do { \
		(PLUGIN)->hub.command_del(PLUGIN, HANDLE); \
		hub_free(HANDLE); \
	} while (0)

int plugin_register(struct plugin_handle* plugin, const char* config)
{
	struct user_manager* manager;
	PLUGIN_INITIALIZE(plugin, "User management plugin", "1.0", "Manage users with hub commands.");

	manager = parse_config(config, plugin);
	if (!manager)
		return -1;

	CMD_ADD(manager->cmd_password, plugin, "password", "+p",   auth_cred_user,     &command_password, "Change your own password.");
	CMD_ADD(manager->cmd_useradd,  plugin, "useradd",  "np?C", auth_cred_operator, &command_useradd,  "Register a new user.");
	CMD_ADD(manager->cmd_userdel,  plugin, "userdel",  "+n",   auth_cred_operator, &command_userdel,  "Delete a registered user.");
	CMD_ADD(manager->cmd_userlist, plugin, "userlist", "?+n",  auth_cred_operator, &command_userlist, "List all registered users or search for users by nick.");
	CMD_ADD(manager->cmd_usermod,  plugin, "usermod",  "nC",   auth_cred_super,    &command_usermod,  "Modify registered user credentials.");
	CMD_ADD(manager->cmd_userpass, plugin, "userpass", "n+p",  auth_cred_operator, &command_userpass, "Change password for a registered user.");

	if (manager->register_self)
		CMD_ADD(manager->cmd_register, plugin, "register", "p", auth_cred_guest,   &command_register, "Register your username.");

	plugin->ptr = manager;

	return 0;
}

int plugin_unregister(struct plugin_handle* plugin)
{
	struct user_manager* manager = (struct user_manager*) plugin->ptr;
	set_error_message(plugin, 0);

	if (manager)
	{
		CMD_DEL(plugin, manager->cmd_register);
		CMD_DEL(plugin, manager->cmd_password);
		CMD_DEL(plugin, manager->cmd_useradd);
		CMD_DEL(plugin, manager->cmd_userdel);
		CMD_DEL(plugin, manager->cmd_userlist);
		CMD_DEL(plugin, manager->cmd_usermod);
		CMD_DEL(plugin, manager->cmd_userpass);

		hub_free(manager);
		plugin->ptr = NULL;
	}

	return 0;
}

