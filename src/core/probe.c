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

#include "uhub.h"
#include "probe.h"

#define PROBE_RECV_SIZE 12

static void probe_handle_adc(struct hub_probe* probe, char* recvbuf, ssize_t recvlen);
static void probe_handle_tls(struct hub_probe* probe, char* recvbuf, ssize_t recvlen);
static void probe_handle_http(struct hub_probe* probe, char* recvbuf, ssize_t recvlen);
static void probe_handle_irc(struct hub_probe* probe, char* recvbuf, ssize_t recvlen);

static inline int probe_is_adc(char* recvbuf, ssize_t recvlen)
{
	return (recvlen >= 4 && memcmp(recvbuf, "HSUP", 4) == 0);
}

static inline int probe_is_tls(char* recvbuf, ssize_t recvlen)
{
	return (recvlen >= 11 &&
		recvbuf[0] == 22 &&
		recvbuf[1] == 3 && /* protocol major version */
		recvbuf[5] == 1 && /* message type */
		recvbuf[9] == recvbuf[1]);
}

static inline int probe_is_http(char* recvbuf, ssize_t recvlen)
{
	/* The smallest HTTP request is "GET / HTTP/1.0\r\n\r\n" (18 chars)
	 * but the probe recv size might be less than that
	 */
	ssize_t min_size = PROBE_RECV_SIZE < 18 ? PROBE_RECV_SIZE : 18;
	if (recvlen < min_size)
		return 0;

	switch (recvbuf[0])
	{
		case 'G':
			return (memcmp(recvbuf, "GET ", 4) == 0);

		case 'P':
			if (memcmp(recvbuf, "PUT ", 4) == 0)
				return 1;
			else if (memcmp(recvbuf, "POST ", 5) == 0)
				return 1;
			else if (memcmp(recvbuf, "PATCH ", 6) == 0)
				return 1;
			else
				return 0;

		case 'H':
			return (memcmp(recvbuf, "HEAD ", 5) == 0);

		case 'O':
			return (memcmp(recvbuf, "OPTIONS ", 8) == 0);

		case 'D':
			return (memcmp(recvbuf, "DELETE ", 7) == 0);

		/* ignore "TRACE" and "CONNECT" */
		default:
			return 0;
	}
}

static inline int probe_is_irc(char* recvbuf, ssize_t recvlen)
{
	return (recvlen >= 4 && memcmp(recvbuf, "NICK", 4) == 0);
}


static void probe_net_event_timeout(struct hub_probe* probe)
{
	int len;
	char buf[512];
	const char* redirect_addr = probe->hub->config->nmdc_redirect_addr;

	/* Send NMDC redirect if configured.
	 *
	 * NMDC is weird, the server is actually the first one to speak, so in
	 * order to detect NMDC connections we have to wait a second or 2 to see
	 * if the client sends us anything. If they don't and it times out, we
	 * have to blindly assume it's an NMDC connection attempt and send the
	 * redirect.
	 */
	if (redirect_addr[0] == '\0')
	{
		LOG_TRACE("Probe timed out");
		probe_destroy(probe);
		return;
	}

	len = snprintf(buf, sizeof(buf), "<hub> Redirecting...|$ForceMove %s|", redirect_addr);
	if (len <= 0)
	{
		LOG_WARN("Error %d (%d) while sending NMDC redirect", len, errno);
	}
	else if ((size_t) len >= sizeof(buf))
	{
		LOG_WARN("NMDC Redirect address is too long: %d/%" PRIsz, len + 1, sizeof(buf));
	}
	else
	{
		net_con_send(probe->connection, buf, (size_t) len);
		LOG_TRACE("Probe timed out, redirecting to %s (via NMDC).", redirect_addr);
	}

	probe_destroy(probe);
}

static void probe_net_event_read(struct hub_probe* probe)
{
	ssize_t bytes;

	char probe_recvbuf[PROBE_RECV_SIZE];
	bytes = net_con_peek(probe->connection, probe_recvbuf, PROBE_RECV_SIZE);

	if (bytes < 4)
		LOG_TRACE("Dropping small probe (%" PRIssz ")", bytes);

	else if (probe_is_adc(probe_recvbuf, bytes))
		probe_handle_adc(probe, probe_recvbuf, bytes);

	else if (probe_is_tls(probe_recvbuf, bytes))
		probe_handle_tls(probe, probe_recvbuf, bytes);

	else if (probe_is_http(probe_recvbuf, bytes))
		probe_handle_http(probe, probe_recvbuf, bytes);

	else if (probe_is_irc(probe_recvbuf, bytes))
		probe_handle_irc(probe, probe_recvbuf, bytes);

	else
	{
		LOG_TRACE("Probed unsupported protocol: %02x%02x %02x%02x.",
			probe_recvbuf[0], probe_recvbuf[1],
			probe_recvbuf[2], probe_recvbuf[3]);
	}

	probe_destroy(probe);
}

static void probe_net_event(struct net_connection* con, int events, void *arg)
{
	struct hub_probe* probe = (struct hub_probe*) net_con_get_ptr(con);
	uhub_assert(con == probe->connection);

	if (events == NET_EVENT_TIMEOUT)
		probe_net_event_timeout(probe);

	else if (events & NET_EVENT_READ)
		probe_net_event_read(probe);
}

struct hub_probe* probe_create(struct hub_info* hub, int sd, struct ip_addr_encap* addr)
{
	struct hub_probe* probe = (struct hub_probe*) hub_malloc_zero(sizeof(struct hub_probe));
	int timeout;

	if (probe == NULL)
		return NULL; /* OOM */

	LOG_TRACE("probe_create(): %p", probe);

	probe->hub = hub;
	probe->connection = net_con_create();
	net_con_initialize(probe->connection, sd, probe_net_event, probe, NET_EVENT_READ);

	if (*hub->config->nmdc_redirect_addr)
		timeout = TIMEOUT_REDIRECT;
	else
		timeout = TIMEOUT_CONNECTED;

	net_con_set_timeout(probe->connection, timeout);

	memcpy(&probe->addr, addr, sizeof(struct ip_addr_encap));
	return probe;
}

void probe_destroy(struct hub_probe* probe)
{
	LOG_TRACE("probe_destroy(): %p (connection=%p)", probe, probe->connection);
	if (probe->connection)
	{
		net_con_close(probe->connection);
		probe->connection = NULL;
	}
	hub_free(probe);
}

static void probe_handle_adc(struct hub_probe* probe, char* recvbuf, ssize_t recvlen)
{
	LOG_TRACE("Probed ADC");

#ifdef SSL_SUPPORT
	/* TLS redirect check */
	if (!net_con_is_ssl(probe->connection) && probe->hub->config->tls_enable && probe->hub->config->tls_require)
	{
		const char* redirect_addr = probe->hub->config->tls_require_redirect_addr;
		if (redirect_addr[0] == '\0')
		{
			LOG_TRACE("TLS is required - closing non-TLS connection.");
		}
		else
		{
			int len;
			char buf[512];

			len = snprintf(buf, sizeof(buf),
					"ISUP " ADC_PROTO_SUPPORT "\n"
					"ISID AAAB\n"
					"IINF NIRedirecting...\n"
					"IQUI AAAB RD%s\n",
					redirect_addr);

			if (len <= 0)
			{
				LOG_WARN("Error %d (%d) while sending TLS redirect", len, errno);
			}
			else if ((size_t) len >= sizeof(buf))
			{
				LOG_WARN("TLS Redirect address is too long: %d/%" PRIsz, len + 1, sizeof(buf));
			}
			else
			{
				net_con_send(probe->connection, buf, (size_t) len);
				LOG_TRACE("TLS is required - redirecting non-TLS connection to %s.", redirect_addr);
			}
		}
	}
	else
#endif
	{
		/* If the user is created, it will handle freeing the connection when the user disconnects.
		 * So for now set the probe connection to NULL so probe_destroy() doesn't destroy the
		 * connection.
		 */
		if (user_create(probe->hub, probe->connection, &probe->addr))
			probe->connection = NULL;
	}
}

static void probe_handle_tls(struct hub_probe* probe, char* recvbuf, ssize_t recvlen)
{
	/* NOTE: to get here, recvlen must be >= 11 */
#ifdef SSL_SUPPORT
	if (probe->hub->config->tls_enable)
	{
		ssize_t err;

		LOG_TRACE("Probed TLS %d.%d connection.",
			(int) recvbuf[9], (int) recvbuf[10]);

		err = net_con_ssl_handshake(probe->connection, net_con_ssl_mode_server, probe->hub->ctx);
		if (err < 0)
		{
			LOG_TRACE("TLS handshake negotiation failed (%" PRIssz ").", err);
			return;
		}

		/* If the user is created, it will handle freeing the connection when the user disconnects.
		 * So for now set the probe connection to NULL so probe_destroy() doesn't destroy the
		 * connection.
		 */
		if (user_create(probe->hub, probe->connection, &probe->addr))
			probe->connection = NULL;
		else
			LOG_TRACE("Failed to create user for TLS connection");
	}
	else
	{
		LOG_TRACE("Probed TLS %d.%d connection - disabled in hub.",
			(int) recvbuf[9], (int) recvbuf[10]);
	}
#else
	LOG_TRACE("Probed TLS %d.%d connection - not supported by hub.",
		(int) recvbuf[9], (int) recvbuf[10]);
#endif
}

static void probe_handle_http(struct hub_probe* probe, char* recvbuf, ssize_t recvlen)
{
	struct hub_config* config = probe->hub->config;
	char* buf;

	if (config->ignore_http)
	{
		LOG_TRACE("Probed HTTP connection - ignoring.");
		return;
	}

	if (config->http_redirect_addr[0] != '\0')
	{
		char const* fmt;
		size_t addr_len;
		size_t allocated;
		int msg_length;
		unsigned long content_length;
		unsigned long body_off;

		LOG_TRACE("Probed HTTP connection - redirecting.");

		addr_len = strlen(config->http_redirect_addr);

		fmt = "HTTP/1.1 307 Temporary Redirect\r\n"
			"Connection: close\r\n"
			"Location: %s\r\n"
			"Content-Type: text/html; charset=utf-8\r\n"
			"Content-Length: %lu\r\n"
			"\r\n"
			"<html>\r\n"
			"<head><title>307 Temporary Redirect</title></head>\r\n"
			"<body>\r\n"
			"<center><h1>307 Temporary Redirect</h1></center>\r\n"
			"<hr><center><a href=\"%s\">Redirect</a></center>\r\n"
			"</body>\r\n"
			"</html>\r\n";

		// the offset in fmt of the http body text
		body_off = (unsigned long)strstr(fmt, "\r\n\r\n") - (unsigned long)fmt + 4;

		// the length of the body after printf
		content_length = strlen(fmt) - body_off + addr_len - 2;

		// The %lu + 2 * %s in fmt give us space for the NUL and content-length
		allocated = strlen(fmt) + (addr_len * 2);
		buf = hub_malloc(allocated);
		if (!buf)
		{
			LOG_ERROR("probe_handle_http(): out of memory");
			return;
		}

		msg_length = snprintf(buf, allocated, fmt, config->http_redirect_addr, content_length, config->http_redirect_addr);
		if (msg_length <= 0)
			LOG_ERROR("probe_handle_http(): snprintf failed, %d (%d)", msg_length, errno);
		else if ((size_t) msg_length >= allocated)
			LOG_ERROR("probe_handle_http(): buffer too small, %d/%" PRIsz, msg_length + 1, allocated);
		else
			net_con_send(probe->connection, buf, (size_t) msg_length);

		hub_free(buf);
	}
	else
	{
		LOG_TRACE("Probed HTTP connection - not supported.");

		// If you change this, ensure that Content-Length is still correct
		buf = "HTTP/1.1 501 Not Implemented\r\n"
			"Connection: close\r\n"
			"Content-Type: text/html; charset=utf-8\r\n"
			"Content-Length: 136\r\n"
			"\r\n"
			"<html>\r\n"
			"<head><title>501 Not Implemented</title></head>\r\n"
			"<body>\r\n"
			"<center><h1>501 Not Implemented</h1></center>\r\n"
			"<hr>\r\n"
			"</body>\r\n"
			"</html>\r\n";

		net_con_send(probe->connection, buf, strlen(buf));
	}
}

static void probe_handle_irc(struct hub_probe* probe, char* recvbuf, ssize_t recvlen)
{
	LOG_TRACE("Probed IRC connection - Not supported");
}
