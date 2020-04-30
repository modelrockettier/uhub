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

#include "uhub.h"
#include "util/credentials.h"
#include "util/misc.h"
#include <sqlite3.h>

// #define DEBUG_SQL

static sqlite3* db = NULL;
static const char* command = NULL;
static const char* filename = NULL;
static const char* binary = NULL;

static void main_usage();

typedef int (*command_func_t)(size_t, const char**);

static int create(size_t argc, const char** argv);
static int list(size_t argc, const char** argv);
static int pass(size_t argc, const char** argv);
static int add(size_t argc, const char** argv);
static int del(size_t argc, const char** argv);
static int mod(size_t argc, const char** argv);
static int help(size_t argc, const char** argv);

#define die(...) \
	do { \
		fprintf(stderr, __VA_ARGS__); \
		exit(1); \
	} while (0)

struct commands
{
	command_func_t handle;
	const char* command;
	size_t min_args;
	size_t max_args;
	const char* usage;
};

static struct commands COMMANDS[] = {
	{ &create, "create", 0, 0, "" },
	{ &list,   "list",   0, 1, "[nick search]" },
	{ &add,    "add",    2, 3, "username password [credentials = user]" },
	{ &del,    "del",    1, 1, "username" },
	{ &mod,    "mod",    2, 2, "username credentials" },
	{ &pass,   "pass",   2, 2, "username password" },
	{ &help,   "help",   0, SIZE_MAX, "" },
	{ &help,   "-h",     0, SIZE_MAX, "" },
};

NO_RETURN static void print_usage(const char* str)
{
	if (str && *str)
		die("Usage: %s filename %s %s\n", binary, command, str);
	else
		die("Usage: %s filename %s\n", binary, command);
}


/**
 * Validate credentials.
 */
static const char* validate_cred(char const* cred_str)
{
	enum auth_credentials cred;
	if (!auth_string_to_cred(cred_str, &cred))
		die("Invalid user credentials.\nMust be one of: 'user', 'operator', 'super', 'admin', or '[op][u]bot'\n");

	/*
	 * Replace the cred_str with its auth_cred_to_string variant in case
	 * the user used an alias like "reg" which becomes "user".
	 */
	cred_str = auth_cred_to_string(cred);

	// credentials that are valid in uhub commands, but not for sqlite users
	switch (cred)
	{
		case auth_cred_none:
		case auth_cred_guest:
		case auth_cred_link:
			die("Cannot use '%s' credentials for a sqlite user.\n", cred_str);

		default:
			return cred_str;
	}
}

static const char* validate_username(const char* username)
{
	char const* tmp;

	// verify length
	if (strlen(username) > MAX_NICK_LEN)
		die("User name is too long.\n");

	/* Nick must not start with a space */
	if (is_white_space(username[0]))
		die("User name cannot start with white space.\n");

	/* Check for ASCII values below 32 */
	for (tmp = username; *tmp; tmp++)
	{
		if ((*tmp < 32) && (*tmp > 0))
			die("User name contains illegal characters.\n");
	}

	if (!is_valid_utf8(username))
		die("User name must be utf-8 encoded.\n");

	return username;
}


static const char* validate_password(const char* password)
{
	if (*password == '\0')
		die("Password can't be empty.\n");

	if (strlen(password) > MAX_PASS_LEN)
		die("Password is too long.\n");

	if (!is_valid_utf8(password))
		die("Password must be utf-8 encoded.\n");

	return password;
}

static void open_database()
{
	int res = sqlite3_open(filename, &db);

	if (res)
		die("Unable to open database: %s (result=%d)\n", filename, res);
}

static int sql_execute(const char* sql, ...)
{
	va_list args;
	char* query;
	char* errMsg;
	int rc;

	va_start(args, sql);
	query = sqlite3_vmprintf(sql, args);
	va_end(args);

#ifdef DEBUG_SQL
	printf("SQL: %s\n", query);
#endif

	open_database();

	rc = sqlite3_exec(db, query, NULL, NULL, &errMsg);
	sqlite3_free(query);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "ERROR: %s\n", errMsg);
		sqlite3_free(errMsg);
	}

	rc = sqlite3_changes(db);
	sqlite3_close(db);
	return rc;
}

static int command_dispatch(struct commands* cmd, size_t argc, const char** argv)
{
	size_t i;
	for (i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-h") == 0)
			print_usage(cmd->usage);
	}

	if (argc < cmd->min_args || argc > cmd->max_args)
		print_usage(cmd->usage);

	return cmd->handle(argc, argv);
}

static int create(size_t argc, const char** argv)
{
	const char* sql = "CREATE TABLE users"
		"("
			"nickname CHAR NOT NULL UNIQUE,"
			"password CHAR NOT NULL,"
			"credentials CHAR NOT NULL DEFAULT 'user',"
			"created TIMESTAMP DEFAULT (DATETIME('NOW')),"
			"activity TIMESTAMP DEFAULT (DATETIME('NOW'))"
		");";

	sql_execute(sql);
	return 0;
}


static int sql_callback_list(void* ptr, int argc, char **argv, char **colName)
{
	int* found = (int*) ptr;
	uhub_assert(strcmp(colName[0], "credentials") == 0);
	uhub_assert(strcmp(colName[1], "nickname") == 0);
	uhub_assert(strcmp(colName[2], "activity") == 0);
	if (argc < 3)
		die("Unknown SQL response format\n");

	const char* cred = argv[0];
	const char* nick = argv[1];
	const char* last_login = argv[2];

	printf("%s\t%s\t%s\n", cred, nick, last_login);
	(*found)++;
	return 0;
}

static int list(size_t argc, const char** argv)
{
	char* errMsg;
	int found = 0;
	int rc;
	char const* search = "";

	if (argc > 0)
		search = argv[0];

	open_database();

	const char* base_query =
		"SELECT credentials, nickname,"
		// when activity == created, user hasn't logged in
		" CASE activity WHEN created THEN 'Never' ELSE datetime(activity, 'localtime') END AS activity"
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
		";";

	char* where = "";
	if (*search)
	{
		where = sqlite3_mprintf("WHERE nickname LIKE '%%%q%%'", search);
		if (!where)
			die("Error allocating memory for sql query");
	}

	char* query = sqlite3_mprintf(base_query, where);
	if (*search)
		sqlite3_free(where);

	if (!query)
		die("Error creating user list query");

#ifdef DEBUG_SQL
	printf("SQL: %s\n", query);
#endif

	printf("%s\t%s\t%s\n", "CREDS", "NICK", "LAST-LOGIN");

	rc = sqlite3_exec(db, query, sql_callback_list, &found, &errMsg);
	sqlite3_free(query);

	if (rc != SQLITE_OK) {
#ifdef DEBUG_SQL
		fprintf(stderr, "SQL: ERROR: %s (%d)\n", errMsg, rc);
#endif
		sqlite3_free(errMsg);
		exit(1);
	}

	sqlite3_close(db);
	return 0;
}


static int add(size_t argc, const char** argv)
{
	char const* user = NULL;
	char const* pass = NULL;
	const char* cred = NULL;
	int rc;

	user = validate_username(argv[0]);
	pass = validate_password(argv[1]);
	cred = validate_cred(argv[2] ? argv[2] : "user");

	rc = sql_execute("INSERT INTO users (nickname, password, credentials) VALUES('%q', '%q', '%q');", user, pass, cred);

	if (rc < 1)
		die("Unable to add user \"%s\"\n", argv[0]);
	else if (rc > 1)
		fprintf(stderr, "Warning: %d users were added.\n", rc);

	return 0;
}

static int mod(size_t argc, const char** argv)
{
	char const* user = NULL;
	const char* cred = NULL;
	int rc;

	user = argv[0];
	cred = validate_cred(argv[1]);

	rc = sql_execute("UPDATE users SET credentials = '%q' WHERE nickname = '%q';", cred, user);

	if (rc < 1)
		die("Unable to set credentials for user \"%s\"\n", argv[0]);
	else if (rc > 1)
		fprintf(stderr, "Warning: %d users were modified.\n", rc);

	return 0;
}

static int pass(size_t argc, const char** argv)
{
	char const* user = NULL;
	char const* pass = NULL;
	int rc;

	user = argv[0];
	pass = validate_password(argv[1]);

	rc = sql_execute("UPDATE users SET password = '%q' WHERE nickname = '%q';", pass, user);

	if (rc < 1)
		die("Unable to change password for user \"%s\"\n", argv[0]);
	else if (rc > 1)
		fprintf(stderr, "Warning: password was changed for %d users.\n", rc);

	return 0;
}


static int del(size_t argc, const char** argv)
{
	char const* user = NULL;
	int rc;

	user = argv[0];

	rc = sql_execute("DELETE FROM users WHERE nickname = '%q';", user);

	if (rc < 1)
		die("Unable to delete user \"%s\".\n", argv[0]);
	else if (rc > 1)
		fprintf(stderr, "Warning: %d users were deleted.\n", rc);

	return 0;
}

static int help(size_t argc, const char** argv)
{
	main_usage();
	return 0;
}

static void main_usage()
{
	printf(
			"Usage: %s filename command [...]\n"
			"\n"
			"Command syntax:\n"
			"  create\n"
			"  add  username password [credentials = user]\n"
			"  del  username\n"
			"  mod  username credentials\n"
			"  pass username password\n"
			"  list\n"
			"\n"
			"Parameters:\n"
			"  'filename' is a database file\n"
			"  'username' is a nickname (UTF-8, up to %i bytes)\n"
			"  'password' is a password (UTF-8, up to %i bytes)\n"
			"  'credentials' is one of 'user', 'op', 'super', 'admin', 'bot', 'ubot', 'opbot', or 'opubot'\n"
			"\n"
		, binary, MAX_NICK_LEN, MAX_PASS_LEN);
}

int main(int argc, char** argv)
{
	size_t n = 0;
	binary = argv[0];
	filename = argv[1];
	command = argv[2];

	if (argc == 1 || strcmp(filename, "-h") == 0)
	{
		main_usage();
		return 0;
	}

	if (argc < 3)
	{
		main_usage();
		return 1;
	}

	for (; n < ARRAY_SIZE(COMMANDS); n++)
	{
		if (!strcmp(command, COMMANDS[n].command))
			return command_dispatch(&COMMANDS[n], argc - 3, (const char**) &argv[3]);
	}

	// Unknown command!
	main_usage();
	return 1;
}
