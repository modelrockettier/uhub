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
#include <sqlite3.h>
#include "util/memory.h"
#include "util/list.h"
#include "util/misc.h"
#include "util/log.h"
#include "util/config_token.h"

// #define DEBUG_SQL

static void set_error_message(struct plugin_handle* plugin, const char* msg)
{
	plugin->error_msg = msg;
}

struct sql_data
{
	int exclusive;
	sqlite3* db;
};

static int null_callback(void* ptr, int argc, char **argv, char **colName) { return 0; }

static int sql_execute(struct sql_data* sql, int (*callback)(void* ptr, int argc, char **argv, char **colName), void* ptr, const char* sql_fmt, ...)
{
	va_list args;
	char* query;
	char* errMsg;
	int rc;

	va_start(args, sql_fmt);
	query = sqlite3_vmprintf(sql_fmt, args);

#ifdef DEBUG_SQL
	printf("SQL: %s\n", query);
#endif

	rc = sqlite3_exec(sql->db, query, callback, ptr, &errMsg);
	sqlite3_free(query);
	if (rc != SQLITE_OK)
	{
#ifdef DEBUG_SQL
		fprintf(stderr, "ERROR: %s\n", errMsg);
#endif
		sqlite3_free(errMsg);
		return -rc;
	}

	rc = sqlite3_changes(sql->db);
	return rc;
}


static struct sql_data* parse_config(const char* line, struct plugin_handle* plugin)
{
	struct sql_data* data = (struct sql_data*) hub_malloc_zero(sizeof(struct sql_data));
	struct cfg_tokens* tokens = cfg_tokenize(line);
	char* token = cfg_token_get_first(tokens);

	if (!data)
		return 0;

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

		if (strcmp(cfg_settings_get_key(setting), "file") == 0)
		{
			if (!data->db)
			{
				if (sqlite3_open(cfg_settings_get_value(setting), &data->db))
				{
					cfg_tokens_free(tokens);
					cfg_settings_free(setting);
					hub_free(data);
					set_error_message(plugin, "Unable to open database file");
					return 0;
				}
			}
		}
		else if (strcmp(cfg_settings_get_key(setting), "exclusive") == 0)
		{
			if (!string_to_boolean(cfg_settings_get_value(setting), &data->exclusive))
				data->exclusive = 1;
		}
		else
		{
			set_error_message(plugin, "Unknown startup parameters given");
			cfg_tokens_free(tokens);
			cfg_settings_free(setting);
			hub_free(data);
			return 0;
		}

		cfg_settings_free(setting);
		token = cfg_token_get_next(tokens);
	}
	cfg_tokens_free(tokens);

	if (!data->db)
	{
		set_error_message(plugin, "No database file is given, use file=<database>");
		hub_free(data);
		return 0;
	}
	return data;
}

struct data_record {
	struct auth_info* data;
	int found;
};

static int get_user_callback(void* ptr, int argc, char **argv, char **colName){
	struct data_record* data = (struct data_record*) ptr;
	int i = 0;
	for (; i < argc; i++) {
		if (strcmp(colName[i], "nickname") == 0)
			strncpy(data->data->nickname, argv[i], MAX_NICK_LEN);
		else if (strcmp(colName[i], "password") == 0)
			strncpy(data->data->password, argv[i], MAX_PASS_LEN);
		else if (strcmp(colName[i], "credentials") == 0)
		{
			auth_string_to_cred(argv[i], &data->data->credentials);
			data->found = 1;
		}
	}

#ifdef DEBUG_SQL
	printf("SQL: nickname=%s, password=%s, credentials=%s\n", data->data->nickname, data->data->password, auth_cred_to_string(data->data->credentials));
#endif
	return 0;
}

static plugin_st get_user(struct plugin_handle* plugin, const char* nickname, struct auth_info* data)
{
	struct sql_data* sql = (struct sql_data*) plugin->ptr;
	struct data_record result;
	char* query;
	char* errMsg = NULL;
	int rc;

	query = sqlite3_mprintf("SELECT * FROM users WHERE nickname='%q';", nickname);
	memset(data, 0, sizeof(struct auth_info));

	result.data = data;
	result.found = 0;

#ifdef DEBUG_SQL
	printf("SQL: %s\n", query);
#endif

	rc = sqlite3_exec(sql->db, query, get_user_callback, &result, &errMsg);
	sqlite3_free(query);
	if (rc != SQLITE_OK) {
#ifdef DEBUG_SQL
		fprintf(stderr, "SQL: ERROR: %s\n", errMsg);
#endif
		sqlite3_free(errMsg);
		return st_default;
	}

	if (result.found)
		return st_allow;

#ifdef DEBUG_SQL
	printf("SQL: User not found: %s\n", nickname);
#endif
	return st_default;
}

static plugin_st register_user(struct plugin_handle* plugin, struct auth_info* user)
{
	struct sql_data* sql = (struct sql_data*) plugin->ptr;
	const char* nick = user->nickname;
	const char* pass = user->password;
	const char* cred = auth_cred_to_string(user->credentials);
	int rc;

	if (sql->exclusive)
		return st_deny;

	rc = sql_execute(sql, null_callback, NULL, "INSERT INTO users (nickname, password, credentials) VALUES('%q', '%q', '%q');", nick, pass, cred);

	if (rc <= 0)
	{
		fprintf(stderr, "Unable to add user \"%s\"\n", nick);
		return st_deny;
	}
	return st_allow;
}

static plugin_st update_user(struct plugin_handle* plugin, struct auth_info* user)
{
	struct sql_data* sql = (struct sql_data*) plugin->ptr;
	const char* nick = user->nickname;
	const char* pass = user->password;
	const char* cred = auth_cred_to_string(user->credentials);
	int rc;

	if (sql->exclusive)
		return st_deny;

	rc = sql_execute(sql, null_callback, NULL, "INSERT OR REPLACE INTO users (nickname, password, credentials) VALUES('%q', '%q', '%q');", nick, pass, cred);

	if (rc <= 0)
	{
		fprintf(stderr, "Unable to update user \"%s\"\n", nick);
		return st_deny;
	}
	return st_allow;
}

static plugin_st delete_user(struct plugin_handle* plugin, struct auth_info* user)
{
	struct sql_data* sql = (struct sql_data*) plugin->ptr;
	const char* nick = user->nickname;
	int rc;

	if (sql->exclusive)
		return st_deny;

	rc = sql_execute(sql, null_callback, NULL, "DELETE FROM users WHERE nickname='%q' LIMIT 1;", nick);

	if (rc <= 0)
	{
		fprintf(stderr, "Unable to update user \"%s\"\n", nick);
		return st_deny;
	}
	return st_allow;
}

int plugin_register(struct plugin_handle* plugin, const char* config)
{
	PLUGIN_INITIALIZE(plugin, "SQLite authentication plugin", "1.1", "Authenticate users based on a SQLite database.");

	// Authentication actions.
	plugin->funcs.auth_get_user = get_user;
	plugin->funcs.auth_register_user = register_user;
	plugin->funcs.auth_update_user = update_user;
	plugin->funcs.auth_delete_user = delete_user;

	plugin->ptr = parse_config(config, plugin);
	if (plugin->ptr)
		return 0;
	return -1;
}

int plugin_unregister(struct plugin_handle* plugin)
{
	struct sql_data* sql;
	set_error_message(plugin, 0);
	sql = (struct sql_data*) plugin->ptr;
	sqlite3_close(sql->db);
	hub_free(sql);
	return 0;
}

