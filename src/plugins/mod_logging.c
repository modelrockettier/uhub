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

#include "system.h"
#include "plugin_api/handle.h"
#include "plugin_api/command_api.h"

#include "util/cbuffer.h"
#include "util/log.h"
#include "util/list.h"
#include "util/memory.h"
#include "util/misc.h"
#include "util/config_token.h"
#ifndef WIN32
#include <syslog.h>
#endif

#define MAX_MSG_SIZE 500

struct log_data
{
	enum {
		mode_mem = 0,
		mode_file,
		mode_syslog
	} logmode;
	char* logfile;
	int fd;
	size_t max_log_entries;
	struct linked_list* messages;
	struct plugin_command_handle* command_log_handle; ///<<< "A handle to the !log command."
	struct plugin_command_handle* command_findlog_handle; ///<<< "A handle to the !findlog command."
};

static void reset(struct log_data* data)
{
	/* set defaults */
	data->logmode = mode_mem;
	data->logfile = NULL;
	data->fd = -1;
	data->max_log_entries = 200;
	data->messages = list_create();
}

static void set_error_message(struct plugin_handle* plugin, const char* msg)
{
	plugin->error_msg = msg;
}

static void read_log_file(struct plugin_handle* plugin, struct log_data* data)
{
	size_t buflen = MAX_MSG_SIZE * 2;
	size_t off = 0;
	size_t len = 0;
	ssize_t bytes_read = 0;
	char* endp;

	// attempt to read in the existing log contents
	int read_fd = open(data->logfile, O_RDONLY);
	if (read_fd == -1)
		return;

	char* buffer = hub_malloc(buflen + 1);
	if (!buffer)
	{
		close(read_fd);
		return;
	}

	buffer[buflen] = '\0';
	bytes_read = read(read_fd, buffer, buflen);

	while (bytes_read > 0)
	{
		bytes_read += (ssize_t) off;

		buffer[bytes_read] = '\0';
		off = 0;

		while ((endp = strchr(&buffer[off], '\n')) != NULL)
		{
			size_t len = (size_t) (endp - &buffer[off]);
			if (len)
				list_append(data->messages, hub_strndup(&buffer[off], len));
			off += len + 1;
		}

		while (list_size(data->messages) > data->max_log_entries)
			list_remove_first(data->messages, hub_free);

		len = strlen(&buffer[off]);
		if (len >= buflen)
		{
			LOG_WARN("Line too long: " PRINTF_SIZE_T "/" PRINTF_SIZE_T, len, buflen);
			list_append(data->messages, hub_strndup(&buffer[off], len));
			len = 0;
		}
		else
		{
			memmove(buffer, &buffer[off], len + 1);
		}

		off = len;

		bytes_read = read(read_fd, &buffer[off], buflen - off);
	}

	if (len)
		list_append(data->messages, hub_strdup(buffer));

	while (list_size(data->messages) > data->max_log_entries)
		list_remove_first(data->messages, hub_free);

	close(read_fd);
	hub_free(buffer);
}

static int log_open_file(struct plugin_handle* plugin, struct log_data* data)
{
	read_log_file(plugin, data);

	int flags = O_CREAT | O_APPEND | O_WRONLY;
	data->fd = open(data->logfile, flags, 0664);
	return (data->fd >= 0);
}

#ifndef WIN32
static int log_open_syslog(struct plugin_handle* plugin)
{
	openlog("uhub", 0, LOG_USER);
	return 1;
}
#endif

static struct log_data* parse_config(const char* line, struct plugin_handle* plugin)
{
	struct log_data* data = (struct log_data*) hub_malloc(sizeof(struct log_data));
	struct cfg_tokens* tokens = cfg_tokenize(line);
	char* token = cfg_token_get_first(tokens);

	if (!data)
	{
		set_error_message(plugin, "OOM");
		cfg_tokens_free(tokens);
		return 0;
	}

	reset(data);

	while (token)
	{
		struct cfg_settings* setting = cfg_settings_split(token);

		if (!setting)
		{
			set_error_message(plugin, "Unable to parse startup parameters");
			cfg_tokens_free(tokens);
			list_destroy(data->messages);
			hub_free(data->logfile);
			hub_free(data);
			return 0;
		}

		if (strcmp(cfg_settings_get_key(setting), "file") == 0)
		{
			hub_free(data->logfile);
			data->logfile = hub_strdup(cfg_settings_get_value(setting));
			data->logmode = mode_file;
		}
#ifndef WIN32
		else if (strcmp(cfg_settings_get_key(setting), "syslog") == 0)
		{
			int use_syslog = 0;
			if (string_to_boolean(cfg_settings_get_value(setting), &use_syslog))
			{
				data->logmode = (use_syslog) ? mode_syslog : mode_file;

				if (use_syslog && data->logfile)
				{
					set_error_message(plugin, "Can't log to both a file and syslog");
					cfg_tokens_free(tokens);
					cfg_settings_free(setting);
					list_destroy(data->messages);
					hub_free(data->logfile);
					hub_free(data);
					return 0;
				}
			}
		}
#endif
		else if (strcmp(cfg_settings_get_key(setting), "max_log_entries") == 0)
		{
			data->max_log_entries = (size_t) uhub_atoi(cfg_settings_get_value(setting));
			// 0 -> unlimited entries
			if (data->max_log_entries == 0)
				data->max_log_entries = SIZE_MAX;
		}
		else
		{
			set_error_message(plugin, "Unknown startup parameters given");
			cfg_tokens_free(tokens);
			cfg_settings_free(setting);
			list_destroy(data->messages);
			hub_free(data->logfile);
			hub_free(data);
			return 0;
		}

		cfg_settings_free(setting);
		token = cfg_token_get_next(tokens);
	}

	cfg_tokens_free(tokens);

	if (data->logmode == mode_file)
	{
		if (!data->logfile)
		{
			set_error_message(plugin, "No log file is given, use file=<path>");
			list_destroy(data->messages);
			hub_free(data);
			return 0;
		}

		if (!log_open_file(plugin, data))
		{
			list_clear(data->messages, &hub_free);
			list_destroy(data->messages);
			hub_free(data->logfile);
			hub_free(data);
			set_error_message(plugin, "Unable to open log file");
			return 0;
		}
	}
#ifndef WIN32
	else if (data->logmode == mode_syslog)
	{
		if (!log_open_syslog(plugin))
		{
			list_destroy(data->messages);
			hub_free(data->logfile);
			hub_free(data);
			set_error_message(plugin, "Unable to open syslog");
			return 0;
		}
	}
#endif
	return data;
}

static void log_close(struct log_data* data)
{
	if (data->messages)
	{
		list_clear(data->messages, hub_free);
		list_destroy(data->messages);
	}

	if (data->logmode == mode_file)
	{
		hub_free(data->logfile);
		close(data->fd);
	}
#ifndef WIN32
	else if (data->logmode == mode_syslog)
	{
		closelog();
	}
#endif
	hub_free(data);
}

static void log_message(struct log_data* data, const char *format, ...)
{
	static char logmsg[MAX_MSG_SIZE];
	struct tm tmp;
	time_t t;
	va_list args;
	ssize_t size = 0;
	size_t ts_len = 0;

	t = time(NULL);
	localtime_r(&t, &tmp);
	strftime(logmsg, sizeof(logmsg), "%Y-%m-%d %H:%M:%S ", &tmp);
	ts_len = strlen(logmsg);

	va_start(args, format);
	size = vsnprintf(logmsg + ts_len, MAX_MSG_SIZE - ts_len, format, args);
	va_end(args);

	// Add the message to the list (without a trailing newline)
	list_append(data->messages, hub_strdup(logmsg));
	if (list_size(data->messages) > data->max_log_entries)
	{
		list_remove_first(data->messages, hub_free);
	}

	// Append a newline to the message
	if ((size_t)size >= (MAX_MSG_SIZE - ts_len - 1))
	{
		LOG_WARN("Log entry exceeded max size: %lld/" PRINTF_SIZE_T,
			(long long)size, (size_t) MAX_MSG_SIZE - ts_len);

		// make sure the msg ends with "...\n" (4 chars + nul)
		strcpy(&logmsg[MAX_MSG_SIZE - 5], "...\n");

		size = MAX_MSG_SIZE - 1;
	}
	else
	{
		size += (ssize_t)ts_len;
		if (logmsg[size - 1] != '\n')
		{
			logmsg[size++] = '\n';
			logmsg[size] = '\0';
		}
	}

	if (data->logmode == mode_file)
	{
		uhub_assert((size_t)size == strlen(logmsg));

		if (write(data->fd, logmsg, size) < size)
		{
			fprintf(stderr, "Unable to write full log. Error=%d: %s\n", errno, strerror(errno));
		}
		else
		{
#ifdef WIN32
			_commit(data->fd);
#else
#if defined _POSIX_SYNCHRONIZED_IO && _POSIX_SYNCHRONIZED_IO > 0
			fdatasync(data->fd);
#else
			fsync(data->fd);
#endif
#endif
		}
	}
#ifndef WIN32
	else if (data->logmode == mode_syslog)
	{
		// Syslog adds its own timestamps, so don't include it in the message
		syslog(LOG_INFO, "%s", &logmsg[ts_len]);
	}
#endif
}

static void log_user_login(struct plugin_handle* plugin, struct plugin_user* user)
{
	const char* cred = auth_cred_to_string(user->credentials);
	const char* addr = plugin->hub.ip_to_string(plugin, &user->addr);
	const char* sid = plugin->hub.sid_to_string(plugin, user->sid);

	log_message(plugin->ptr, "LoginOK     %s/%s %s \"%s\" (%s) \"%s\"",
		sid, user->cid, addr, user->nick, cred, user->user_agent);
}

static void log_user_login_error(struct plugin_handle* plugin, struct plugin_user* user, const char* reason)
{
	const char* addr = plugin->hub.ip_to_string(plugin, &user->addr);
	const char* sid = plugin->hub.sid_to_string(plugin, user->sid);

	log_message(plugin->ptr, "LoginError  %s/%s %s \"%s\" (%s) \"%s\"",
		sid, user->cid, addr, user->nick, reason, user->user_agent);
}

static void log_user_logout(struct plugin_handle* plugin, struct plugin_user* user, const char* reason)
{
	const char* addr = plugin->hub.ip_to_string(plugin, &user->addr);
	const char* sid = plugin->hub.sid_to_string(plugin, user->sid);

	log_message(plugin->ptr, "Logout      %s/%s %s \"%s\" (%s) \"%s\"",
		sid, user->cid, addr, user->nick, reason, user->user_agent);
}

static void log_change_nick(struct plugin_handle* plugin, struct plugin_user* user, const char* new_nick)
{
	const char* addr = plugin->hub.ip_to_string(plugin, &user->addr);
	const char* sid = plugin->hub.sid_to_string(plugin, user->sid);

	log_message(plugin->ptr, "NickChange  %s/%s %s \"%s\" -> \"%s\"",
		sid, user->cid, addr, user->nick, new_nick);
}

/**
 * Send a status message back to the user who issued the !history command.
 */
static int command_status(struct plugin_handle* plugin, struct plugin_user* user, struct plugin_command* cmd, struct cbuffer* buf)
{
	struct cbuffer* msg = cbuf_create(cbuf_size(buf) + strlen(cmd->prefix) + 8);
	cbuf_append_format(msg, "*** %s: %s", cmd->prefix, cbuf_get(buf));
	plugin->hub.send_message(plugin, user, cbuf_get(msg));
	cbuf_destroy(msg);
	cbuf_destroy(buf);
	return 0;
}

/**
 * Show the user log.
 */
static int command_log(struct plugin_handle* plugin, struct plugin_user* user, struct plugin_command* cmd)
{
	struct log_data* data = (struct log_data*) plugin->ptr;
	struct cbuffer* buf;
	char* msg;

	if (!list_size(data->messages))
	{
		return command_status(plugin, user, cmd, cbuf_create_const("No entries logged."));
	}

	struct plugin_command_arg_data* arg1 = plugin->hub.command_arg_next(plugin, cmd, plugin_cmd_arg_type_integer);
	struct plugin_command_arg_data* arg2 = plugin->hub.command_arg_next(plugin, cmd, plugin_cmd_arg_type_integer);
	size_t offset      = (size_t) (arg1 ? arg1->data.integer : 0);
	size_t max_entries = (size_t) (arg2 ? arg2->data.integer : 20);
	size_t entries = 0;
	size_t first_entry = 0;
	size_t last_entry = 0;

	buf = cbuf_create(128);
	cbuf_append_format(buf, "Logged entries: " PRINTF_SIZE_T " - " PRINTF_SIZE_T " of " PRINTF_SIZE_T,
		offset + 1, offset + max_entries, list_size(data->messages));

	command_status(plugin, user, cmd, buf);

	if (max_entries > 200 || max_entries == 0)
		max_entries = 200;

	// Offset is past the end
	if (offset >= list_size(data->messages))
		return 0;

	// list is from oldest to newest but offset is from newest to oldest
	last_entry = list_size(data->messages) - offset - 1;
	// make sure first_entry doesn't go negative (or wrap around)
	if (last_entry > (max_entries - 1))
		first_entry = last_entry - (max_entries - 1);

	buf = cbuf_create(MAX_MSG_SIZE);
	LIST_FOREACH(char*, msg, data->messages,
	{
		if (entries++ < first_entry)
			continue;

		cbuf_append_format(buf, "* %s", msg);

		if (entries > last_entry)
			cbuf_append(buf, "\n");

		plugin->hub.send_message(plugin, user, cbuf_get(buf));
		cbuf_clear(buf);

		if (entries > last_entry)
			break;
	});

	cbuf_destroy(buf);

	return 0;
}

/**
 * Search through the user log.
 */
static int command_findlog(struct plugin_handle* plugin, struct plugin_user* user, struct plugin_command* cmd)
{
	struct log_data* data = (struct log_data*) plugin->ptr;
	struct cbuffer* buf;
	char* msg;

	if (!list_size(data->messages))
		return command_status(plugin, user, cmd, cbuf_create_const("No entries logged."));

	struct plugin_command_arg_data* arg1 = plugin->hub.command_arg_next(plugin, cmd, plugin_cmd_arg_type_string);
	char* search = arg1->data.string;
	size_t entries = 0;

	buf = cbuf_create(128);
	cbuf_append_format(buf, "Logged entries: " PRINTF_SIZE_T ", searching for \"%s\"",
		list_size(data->messages), search);

	command_status(plugin, user, cmd, buf);

	buf = cbuf_create(MAX_MSG_SIZE);
	LIST_FOREACH(char*, msg, data->messages,
	{
		if (strstr(msg, search) == NULL)
			continue;

		entries++;

		cbuf_append_format(buf, "* %s", msg);

		plugin->hub.send_message(plugin, user, cbuf_get(buf));
		cbuf_clear(buf);
	});

	cbuf_append_format(buf, PRINTF_SIZE_T " entries shown.\n", entries);
	command_status(plugin, user, cmd, buf);

	return 0;
}

int plugin_register(struct plugin_handle* plugin, const char* config)
{
	struct log_data* data;

	PLUGIN_INITIALIZE(plugin, "Logging plugin", "1.0", "Logs users entering and leaving the hub.");

	plugin->funcs.on_user_login = log_user_login;
	plugin->funcs.on_user_login_error = log_user_login_error;
	plugin->funcs.on_user_logout = log_user_logout;
	plugin->funcs.on_user_nick_change = log_change_nick;

	data = parse_config(config, plugin);
	if (!data)
		return -1;

	plugin->ptr = data;

	data->command_log_handle = (struct plugin_command_handle*) hub_malloc(sizeof(struct plugin_command_handle));
	PLUGIN_COMMAND_INITIALIZE(data->command_log_handle, plugin, "log", "?N?N", auth_cred_operator, &command_log, "Show user logs.");
	plugin->hub.command_add(plugin, data->command_log_handle);

	data->command_findlog_handle = (struct plugin_command_handle*) hub_malloc(sizeof(struct plugin_command_handle));
	PLUGIN_COMMAND_INITIALIZE(data->command_findlog_handle, plugin, "findlog", "m", auth_cred_operator, &command_findlog, "Search user logs.");
	plugin->hub.command_add(plugin, data->command_findlog_handle);

	return 0;
}

int plugin_unregister(struct plugin_handle* plugin)
{
	struct log_data* data = (struct log_data*) plugin->ptr;

	plugin->hub.command_del(plugin, data->command_findlog_handle);
	hub_free(data->command_findlog_handle);

	plugin->hub.command_del(plugin, data->command_log_handle);
	hub_free(data->command_log_handle);

	log_close(data);

	return 0;
}

