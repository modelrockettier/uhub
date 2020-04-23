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

struct auth_sqlite
{
	sqlite3* db;
	int exclusive; ///<<< "This is the only pdata plugin"
	int readonly; ///<<< "Do not modify the user database"
	int update_activity; ///<<< "Update the user's activity timestamp when they log in"
	char journal[16]; ///<<< "The SQLite journal mode to use"
};

static int sql_execute(struct auth_sqlite* pdata, int (*callback)(void* ptr, int argc, char **argv, char **colName), void* ptr, const char* sql_fmt, ...)
{
	va_list args;
	char const* query;
	char* errMsg;
	int rc;

	va_start(args, sql_fmt);
	query = sqlite3_vmprintf(sql_fmt, args);
	va_end(args);

	if (!query)
		return -SQLITE_NOMEM;

#ifdef DEBUG_SQL
	fprintf(stderr, "SQL: %s\n", query);
#endif

	rc = sqlite3_exec(pdata->db, query, callback, ptr, &errMsg);

	sqlite3_free((char*) query);

	if (rc != SQLITE_OK)
	{
#ifdef DEBUG_SQL
		fprintf(stderr, "ERROR: %s\n", errMsg);
#endif
		sqlite3_free(errMsg);
		return -rc;
	}

	rc = sqlite3_changes(pdata->db);
	return rc;
}

static void sqlite_setup(struct plugin_handle* plugin)
{
	struct auth_sqlite* pdata = (struct auth_sqlite*) plugin->ptr;
	int rc;

	const char* table_create = "CREATE TABLE IF NOT EXISTS users"
		"("
			"nickname CHAR NOT NULL UNIQUE,"
			"password CHAR NOT NULL,"
			"credentials CHAR NOT NULL DEFAULT 'user',"
			"created TIMESTAMP DEFAULT (DATETIME('NOW')),"
			"activity TIMESTAMP DEFAULT (DATETIME('NOW'))"
		");";

	if (!pdata->readonly)
	{
		rc = sql_execute(pdata, NULL, NULL, table_create);
		if (rc < 0)
		{
			LOG_ERROR("mod_auth_sqlite: failed to create the users table: %s",
				sqlite3_errstr(-rc));
		}
	}

	// set the sqlite journal mode if it's not an empty string, see:
	// https://www.sqlite.org/pragma.html#pragma_journal_mode
	if (pdata->journal[0])
	{
		rc = sql_execute(pdata, NULL, NULL, "PRAGMA journal_mode=%s;", pdata->journal);
		if (rc < 0)
		{
			LOG_ERROR("mod_auth_sqlite: failed to set the database journal mode to \"%s\": %s",
				pdata->journal, sqlite3_errstr(-rc));
		}
	}

	// warn if we can't query the database
	const char* query_check = "SELECT nickname FROM users LIMIT 1;";
	rc = sql_execute(pdata, NULL, NULL, query_check);
	if (rc < 0)
		LOG_ERROR("mod_auth_sqlite: failed to query database: %s", sqlite3_errstr(-rc));
}

static struct auth_sqlite* parse_config(const char* line, struct plugin_handle* plugin)
{
	struct auth_sqlite* pdata = (struct auth_sqlite*) hub_malloc_zero(sizeof(struct auth_sqlite));
	struct cfg_tokens* tokens = cfg_tokenize(line);
	char* token = cfg_token_get_first(tokens);
	char* file = NULL;
	int flags;
	int rc;

	if (!pdata)
	{
		set_error_message(plugin, "OOM");
		cfg_tokens_free(tokens);
		return NULL;
	}

	pdata->exclusive = 0;
	pdata->readonly = 0;
	pdata->update_activity = 2; // default value = on, but different from manually set
	strcpy(pdata->journal, "");

	while (token)
	{
		struct cfg_settings* setting = cfg_settings_split(token);

		if (!setting)
		{
			set_error_message(plugin, "Unable to parse startup parameters");
			cfg_tokens_free(tokens);
			hub_free(file);
			hub_free(pdata);
			return NULL;
		}

		if (strcmp(cfg_settings_get_key(setting), "file") == 0)
		{
			if (file)
			{
				set_error_message(plugin, "Only 1 database file is allowed");
				cfg_tokens_free(tokens);
				cfg_settings_free(setting);
				hub_free(file);
				hub_free(pdata);
				return NULL;
			}

			file = hub_strdup(cfg_settings_get_value(setting));
			if (!file)
			{
				cfg_tokens_free(tokens);
				cfg_settings_free(setting);
				hub_free(pdata);
				set_error_message(plugin, "No memory");
				return NULL;
			}
		}
		else if (strcmp(cfg_settings_get_key(setting), "journal") == 0)
		{
			size_t jsz = sizeof(pdata->journal);
			size_t len = strlcpy(pdata->journal, cfg_settings_get_value(setting), jsz);

			if (len >= jsz)
			{
				cfg_tokens_free(tokens);
				cfg_settings_free(setting);
				hub_free(file);
				hub_free(pdata);
				set_error_message(plugin, "Invalid journal setting");
				return NULL;
			}
		}
		else if (strcmp(cfg_settings_get_key(setting), "exclusive") == 0)
		{
			if (!string_to_boolean(cfg_settings_get_value(setting), &pdata->exclusive))
				pdata->exclusive = 1;
		}
		else if (strcmp(cfg_settings_get_key(setting), "readonly") == 0)
		{
			if (!string_to_boolean(cfg_settings_get_value(setting), &pdata->readonly))
				pdata->readonly = 1;
		}
		else if (strcmp(cfg_settings_get_key(setting), "update_activity") == 0)
		{
			if (!string_to_boolean(cfg_settings_get_value(setting), &pdata->update_activity))
				pdata->update_activity = 1;
		}
		else
		{
			set_error_message(plugin, "Unknown startup parameters given");
			cfg_tokens_free(tokens);
			cfg_settings_free(setting);
			hub_free(file);
			hub_free(pdata);
			return NULL;
		}

		cfg_settings_free(setting);
		token = cfg_token_get_next(tokens);
	}
	cfg_tokens_free(tokens);

	if (!file)
	{
		set_error_message(plugin, "No database file is given, use file=<database>");
		hub_free(pdata);
		return NULL;
	}

	if (pdata->readonly)
	{
		LOG_INFO("mod_auth_sqlite: The readonly flag is set, not user modifications are allowed");

		if (pdata->update_activity == 1)
			LOG_WARN("mod_auth_sqlite: User activity updates disabled: readonly database.");

		pdata->update_activity = 0;

		flags = SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READONLY;
	}
	else
		flags = SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

	rc = sqlite3_open_v2(file, &pdata->db, flags, NULL);
	hub_free(file);

	if (rc != SQLITE_OK)
	{
		hub_free(pdata);
		set_error_message(plugin, "Unable to open database file");
		return NULL;
	}

	return pdata;
}

struct data_record {
	struct auth_info* userinfo;
	int found;
};

static int get_user_callback(void* ptr, int argc, char **argv, char **colName){
	struct data_record* rec = (struct data_record*) ptr;
	struct auth_info* data;
	int i = 0;
	size_t max;
	size_t len;

	if (!ptr)
		return 0;

	if (argc >= 1)
		rec->found = 1;

	data = rec->userinfo;

	if (!data)
		return 0;

	data->activity[0] = '\0';
	data->nickname[0] = '\0';
	data->password[0] = '\0';
	data->credentials = auth_cred_none;

	uhub_assert(((size_t) -1) > ((size_t) 0));

	for (; i < argc; i++) {
		/* Length overrun warnings are only for columns that opt-in below */
		len = 0;
		max = 1;

		if (strcmp(colName[i], "activity") == 0)
		{
			max = MAX_ACTIVITY_LEN + 1;
			len = strlcpy(data->activity, argv[i], max);
			uhub_assert(sizeof(data->activity) == max);
		}
		else if (strcmp(colName[i], "credentials") == 0)
		{
			int ok = auth_string_to_cred(argv[i], &data->credentials);
			if (!ok)
			{
				LOG_ERROR("Unknown credential level \"%s\" found in get_user results", argv[i]);
				return -1;
			}
			else if (data->credentials < auth_cred_user)
				LOG_WARN("Found a guest in the database");
		}
		else if (strcmp(colName[i], "nickname") == 0)
		{
			max = MAX_NICK_LEN + 1;
			len = strlcpy(data->nickname, argv[i], max);
			uhub_assert(sizeof(data->nickname) == max);
		}
		else if (strcmp(colName[i], "password") == 0)
		{
			max = MAX_PASS_LEN + 1;
			len = strlcpy(data->password, argv[i], max);
			uhub_assert(sizeof(data->password) == max);
		}
		else
		{
			LOG_ERROR("Unknown column \"%s\" in get_user results", colName[i]);
			return -1;
		}

		if (len >= max)
		{
			LOG_ERROR("Column \"%s\" data too long (%d/%d)", colName[i],
				(int) len + 1, (int) max);
			return -1;
		}
	}

#ifdef DEBUG_SQL
	fprintf(stderr, "SQL: nickname=%s, credentials=%s\n",
		data->nickname, auth_cred_to_string(data->credentials));
#endif
	return 0;
}

static int get_user_list_callback(void* ptr, int argc, char **argv, char **colName){
	struct linked_list* users = (struct linked_list*) ptr;
	struct data_record rec = { 0, };
	int rc;

	if (!ptr)
		return 0;

	rec.userinfo = hub_malloc(sizeof(struct auth_info));
	if (!rec.userinfo)
	{
		LOG_ERROR("get_user_list_callback(): OOM");
		list_clear(users, &hub_free);
		return -1;
	}

	rc = get_user_callback(&rec, argc, argv, colName);
	if (rc != 0)
	{
		list_clear(users, &hub_free);
		hub_free(rec.userinfo);
		return rc;
	}

	list_append(users, rec.userinfo);
	return 0;
}

static plugin_st get_user(struct plugin_handle* plugin, const char* nickname, struct auth_info* userinfo)
{
	struct auth_sqlite* pdata = (struct auth_sqlite*) plugin->ptr;
	struct data_record result;
	char* query;
	char* errMsg = NULL;
	plugin_st fail = (pdata->exclusive) ? st_deny : st_default;
	int rc;

	result.found = 0;
	result.userinfo = userinfo;

	if (userinfo)
		memset(userinfo, 0, sizeof(struct auth_info));

	const char* query_fmt =
		"SELECT credentials,nickname,password,"
		// when activity == created, user hasn't logged in
		" CASE activity WHEN created THEN 'Never' ELSE datetime(activity, 'localtime') END AS activity"
		" FROM users"
		" WHERE nickname='%q'"
		" LIMIT 1"
		";";

	query = sqlite3_mprintf(query_fmt, nickname);
	if (!query) // OOM
	{
		LOG_ERROR("mod_auth_sqlite: OOM");
		return fail;
	}

#ifdef DEBUG_SQL
	fprintf(stderr, "SQL: %s\n", query);
#endif

	rc = sqlite3_exec(pdata->db, query, get_user_callback, &result, &errMsg);
	sqlite3_free(query);
	if (rc != SQLITE_OK) {
#ifdef DEBUG_SQL
		fprintf(stderr, "SQL: ERROR: %s\n", errMsg);
#endif
		sqlite3_free(errMsg);
		return fail;
	}

	if (result.found)
		return st_allow;

#ifdef DEBUG_SQL
	fprintf(stderr, "SQL: User not found: %s\n", nickname);
#endif

	return fail;
}

static plugin_st get_user_list(struct plugin_handle* plugin, const char* search, struct linked_list* users)
{
	struct auth_sqlite* pdata = (struct auth_sqlite*) plugin->ptr;
	int rc;

	const char* query =
		"SELECT credentials,nickname,password,"
		"CASE activity WHEN created THEN 'Never' ELSE datetime(activity, 'localtime') END AS activity"
		" FROM users"
		" %s"
		" ORDER BY"
		// order by the credential strings: users first, then bots
		// within users and bots, higher privileges are first
		" CASE credentials"
		// users
		"  WHEN 'admin'    THEN 0"
		"  WHEN 'super'    THEN 1"
		"  WHEN 'op'       THEN 2" // older uhub-passwd versions used "op"
		"  WHEN 'operator' THEN 2" // auth_cred_to_string() uses "operator"
		"  WHEN 'user'     THEN 3"
		// bots
		"  WHEN 'link'     THEN 4"
		"  WHEN 'opubot'   THEN 5"
		"  WHEN 'opbot'    THEN 6"
		"  WHEN 'ubot'     THEN 7"
		"  WHEN 'bot'      THEN 8"
		"  ELSE       credentials"
		" END"
		" LIMIT 100"
		";";

	char* where = "";
	if (search && *search)
	{
		where = sqlite3_mprintf("WHERE nickname LIKE '%%%q%%'", search);
		if (!where)
		{
			LOG_ERROR("mod_auth_sqlite: OOM");
			return st_deny;
		}
	}

	rc = sql_execute(pdata, get_user_list_callback, users, query, where);
	if (search && *search)
		sqlite3_free(where);

	if (rc < 0)
	{
		LOG_ERROR("mod_auth_sqlite: failed to get user list: %s", sqlite3_errstr(-rc));
		return st_deny;
	}

	return (pdata->exclusive) ? st_allow : st_default;
}

static plugin_st register_user(struct plugin_handle* plugin, struct auth_info* userinfo)
{
	struct auth_sqlite* pdata = (struct auth_sqlite*) plugin->ptr;
	const char* nick = userinfo->nickname;
	const char* pass = userinfo->password;
	const char* cred = auth_cred_to_string(userinfo->credentials);
	plugin_st fail = (pdata->exclusive) ? st_deny : st_default;
	int rc;

	if (pdata->readonly)
		return fail;

	// we only handle registered users
	if (userinfo->credentials < auth_cred_user)
		return fail;

	const char* query = "INSERT INTO users (nickname, password, credentials) VALUES('%q', '%q', '%q');";
	rc = sql_execute(pdata, NULL, NULL, query, nick, pass, cred);

	if (rc <= 0)
	{
		LOG_ERROR("Unable to add user \"%s\": %s\n", nick, sqlite3_errstr(-rc));
		return fail;
	}
	return st_allow;
}

static plugin_st update_user(struct plugin_handle* plugin, struct auth_info* userinfo)
{
	struct auth_sqlite* pdata = (struct auth_sqlite*) plugin->ptr;
	const char* nick = userinfo->nickname;
	const char* pass = userinfo->password;
	const char* cred = auth_cred_to_string(userinfo->credentials);
	plugin_st fail = (pdata->exclusive) ? st_deny : st_default;
	int rc;

	if (pdata->readonly)
		return fail;

	// we only handle registered users
	if (userinfo->credentials < auth_cred_user)
		return fail;

	const char* query = "UPDATE users SET password='%q', credentials='%q' WHERE nickname='%q';";
	rc = sql_execute(pdata, NULL, NULL, query, pass, cred, nick);

	if (rc <= 0)
	{
		LOG_ERROR("Unable to update user \"%s\": %s\n", nick, sqlite3_errstr(-rc));
		return fail;
	}
	else if (rc > 1)
	{
		LOG_WARN("Updated %d users! \"%s\"\n", rc, nick);
	}
	return st_allow;
}

static plugin_st delete_user(struct plugin_handle* plugin, struct auth_info* userinfo)
{
	struct auth_sqlite* pdata = (struct auth_sqlite*) plugin->ptr;
	const char* nick = userinfo->nickname;
	plugin_st fail = (pdata->exclusive) ? st_deny : st_default;
	int rc;

	if (pdata->readonly)
		return fail;

	const char* query = "DELETE FROM users WHERE nickname='%q';";
	rc = sql_execute(pdata, NULL, NULL, query, nick);

	if (rc <= 0)
	{
		LOG_ERROR("Unable to delete user \"%s\": %s\n", nick, sqlite3_errstr(-rc));
		return fail;
	}
	else if (rc > 1)
	{
		LOG_WARN("Deleted %d users! \"%s\"\n", rc, nick);
	}
	return st_allow;
}

// This can sometimes take over 60 ms on some systems, so high-traffic hubs may
// want to disable this (set the update_activity parameter to 0)
static void update_user_activity(struct plugin_handle* plugin, struct plugin_user* user)
{
	struct auth_sqlite* pdata = (struct auth_sqlite*) plugin->ptr;

	if (user->credentials > auth_cred_guest)
	{
		const char* query = "UPDATE users SET activity=DATETIME('NOW') WHERE nickname='%q';";
		int rc = sql_execute(pdata, NULL, NULL, query, user->nick);

		if (rc < 0 || (rc == 0 && pdata->exclusive))
		{
			LOG_ERROR("Unable to update login activity for user \"%s\": %s\n",
				user->nick, sqlite3_errstr(-rc));
		}
		else if (rc > 1)
		{
			LOG_WARN("Updated login activity for %d users! \"%s\"\n",
				rc, user->nick);
		}
	}
}

PLUGIN_API int plugin_register(struct plugin_handle* plugin, const char* config)
{
	struct auth_sqlite* pdata;
	PLUGIN_INITIALIZE(plugin, "SQLite authentication plugin", "1.1", "Authenticate users with a SQLite database.");

	pdata = parse_config(config, plugin);
	if (!pdata)
		return -1;

	// Authentication actions.
	plugin->funcs.auth_get_user = get_user;
	plugin->funcs.auth_get_user_list = get_user_list;
	plugin->funcs.auth_register_user = register_user;
	plugin->funcs.auth_update_user = update_user;
	plugin->funcs.auth_delete_user = delete_user;

	// Log functions
	if (pdata->update_activity) // note: readonly disables update_activity
		plugin->funcs.on_user_login = update_user_activity;

	plugin->ptr = pdata;

	sqlite_setup(plugin);

	return 0;
}

PLUGIN_API int plugin_unregister(struct plugin_handle* plugin)
{
	struct auth_sqlite* pdata = (struct auth_sqlite*) plugin->ptr;
	set_error_message(plugin, 0);

	if (pdata)
	{
		sqlite3_close(pdata->db);
		hub_free(pdata);
		plugin->ptr = NULL;
	}

	return 0;
}

