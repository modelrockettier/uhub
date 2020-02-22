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
static char probe_recvbuf[PROBE_RECV_SIZE];

static void probe_handle_http(struct net_connection* con);

static void probe_net_event(struct net_connection* con, int events, void *arg)
{
	struct hub_probe* probe = (struct hub_probe*) net_con_get_ptr(con);
	if (events == NET_EVENT_TIMEOUT)
	{
		/* Send NMDC redirect if configured.
		 *
		 * NMDC is weird, the server is actually the first one to speak, so in
		 * order to detect NMDC connections we have to wait a second or 2 to
		 * see if the client sends us anything. If they don't and it times out,
		 * we have to blindly assume it's an NMDC connection attempt.
		 */
		if (*probe->hub->config->nmdc_redirect_addr)
		{
			char buf[512];
			int len = snprintf(buf, sizeof(buf), "<hub> Redirecting...|$ForceMove %s|", probe->hub->config->nmdc_redirect_addr);
			if (len <= 0)
			{
				LOG_WARN("Error %d while sending NMDC redirect", len);
			}
			else if (len >= sizeof(buf))
			{
				LOG_WARN("NMDC Redirect address is too long: %d", len);
			}
			else
			{
				net_con_send(con, buf, (size_t) len);
				LOG_TRACE("Probe timed out, redirecting to %s (via NMDC).", probe->hub->config->nmdc_redirect_addr);
			}
		}
		else
		{
			LOG_TRACE("Probe timed out");
		}

		probe_destroy(probe);
		return;
	}

	if (events & NET_EVENT_READ)
	{
		int bytes = net_con_peek(con, probe_recvbuf, PROBE_RECV_SIZE);
		if (bytes < 0)
		{
			probe_destroy(probe);
			return;
		}

		if (bytes >= 4)
		{
			if (memcmp(probe_recvbuf, "HSUP", 4) == 0)
			{
				LOG_TRACE("Probed ADC");
#ifdef SSL_SUPPORT
				if (probe->hub->config->tls_enable && probe->hub->config->tls_require)
				{
					if (*probe->hub->config->tls_require_redirect_addr)
					{
						char buf[512];
						ssize_t len = snprintf(buf, sizeof(buf), "ISUP " ADC_PROTO_SUPPORT "\nISID AAAB\nIINF NIRedirecting...\nIQUI AAAB RD%s\n", probe->hub->config->tls_require_redirect_addr);
						net_con_send(con, buf, (size_t) len);
						LOG_TRACE("Not TLS connection - Redirecting to %s.", probe->hub->config->tls_require_redirect_addr);
					}
					else
					{
						LOG_TRACE("Not TLS connection - closing connection.");
					}
				}
				else
#endif
				if (user_create(probe->hub, probe->connection, &probe->addr))
				{
					probe->connection = 0;
				}
				probe_destroy(probe);
				return;
			}
			else if (bytes >= 11 &&
				probe_recvbuf[0] == 22 &&
				probe_recvbuf[1] == 3 && /* protocol major version */
				probe_recvbuf[5] == 1 && /* message type */
				probe_recvbuf[9] == probe_recvbuf[1])
			{
#ifdef SSL_SUPPORT
				if (probe->hub->config->tls_enable)
				{
					LOG_TRACE("Probed TLS %d.%d connection", (int) probe_recvbuf[9], (int) probe_recvbuf[10]);
					if (net_con_ssl_handshake(con, net_con_ssl_mode_server, probe->hub->ctx) < 0)
					{
						LOG_TRACE("TLS handshake negotiation failed.");
					}
					else if (user_create(probe->hub, probe->connection, &probe->addr))
					{
						probe->connection = 0;
					}
				}
				else
				{
					LOG_TRACE("Probed TLS %d.%d connection. TLS disabled in hub.", (int) probe_recvbuf[9], (int) probe_recvbuf[10]);
				}
#else
				LOG_TRACE("Probed TLS %d.%d connection. TLS not supported by hub.", (int) probe_recvbuf[9], (int) probe_recvbuf[10]);
#endif
				probe_destroy(probe);
				return;
			}
			else if ((memcmp(probe_recvbuf, "GET ", 4) == 0) ||
				 (memcmp(probe_recvbuf, "POST", 4) == 0) ||
				 (memcmp(probe_recvbuf, "HEAD", 4) == 0) ||
				 (memcmp(probe_recvbuf, "OPTI", 4) == 0))
			{
				/* Looks like HTTP. */
				LOG_TRACE("Probed HTTP connection. Not supported closing connection (%s)", ip_convert_to_string(&probe->addr));
				probe_handle_http(con);
				probe_destroy(probe);
			}
			else if (memcmp(probe_recvbuf, "NICK", 4) == 0)
			{
				/* Looks like IRC - Not supported, but we log it. */
				LOG_TRACE("Probed IRC connection. Not supported closing connection (%s)", ip_convert_to_string(&probe->addr));
				probe_destroy(probe);
			}
			else
			{
				LOG_TRACE("Probed unsupported protocol: %02x%02x %02x%02x.", (int) probe_recvbuf[0], (int) probe_recvbuf[1], (int) probe_recvbuf[2], (int) probe_recvbuf[3]);
			}
			probe_destroy(probe);
			return;
		}
	}
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
		probe->connection = 0;
	}
	hub_free(probe);
}

static void probe_handle_http(struct net_connection* con)
{
	struct hub_probe* probe = (struct hub_probe*) net_con_get_ptr(con);
	struct hub_config* config = probe->hub->config;
	size_t addr_len;
	char* buf;

	if (config->ignore_http)
		return;

	addr_len = strlen(config->http_redirect_addr);
	if (addr_len != 0)
	{
		char const* fmt;
		size_t allocated;
		int len;
		unsigned long content_length;
		unsigned long body_off;

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

		len = snprintf(buf, allocated, fmt, config->http_redirect_addr, content_length, config->http_redirect_addr);
		if (len <= 0)
			LOG_ERROR("probe_handle_http(): snprintf failed, %d", len);
		else if ((size_t) len >= allocated)
			LOG_ERROR("probe_handle_http(): buffer too small, %d/" PRINTF_SIZE_T, len + 1, allocated);
		else
			net_con_send(con, buf, len);

		hub_free(buf);
	}
	else
	{
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

		net_con_send(con, buf, strlen(buf));
	}
}
