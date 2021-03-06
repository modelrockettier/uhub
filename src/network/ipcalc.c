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

int ip_is_valid_ipv4(const char* address)
{
	size_t i = 0; /* address index */
	int o = 0; /* octet number */
	int n = 0; /* numbers after each dot */
	int d = 0; /* dots */

	if (!address || strlen(address) > 15 || strlen(address) < 7)
		return 0;

	for (; i < strlen(address); i++)
	{
		if (is_num(address[i]))
		{
			n++;
			o *= 10;
			o += (address[i] - '0');
		}
		else if (address[i] == '.')
		{
			if (n == 0 || n > 3 || o > 255)
				return 0;

			n = 0;
			o = 0;
			d++;
		}
		else
		{
			return 0;
		}
	}

	if (n == 0 || n > 3 || o > 255 || d != 3)
		return 0;
	return 1;
}


int ip_is_valid_ipv6(const char* address)
{
	unsigned char buf[16];
	int ret = net_string_to_address(AF_INET6, address, buf);
	if (ret <= 0)
		return 0;
	return 1;
}


int ip_convert_to_binary(const char* taddr, struct ip_addr_encap* raw)
{
	if (net_string_to_address(AF_INET6, taddr, &raw->internal_ip_data.in6) > 0)
	{
		raw->af = AF_INET6;
		return AF_INET6;
	}
	else if (net_string_to_address(AF_INET, taddr, &raw->internal_ip_data.in) > 0)
	{
		raw->af = AF_INET;
		return AF_INET;
	}

	return -1;
}


const char* ip_convert_to_string(struct ip_addr_encap* raw)
{
	static char address[INET6_ADDRSTRLEN + 1];
	memset(address, 0, INET6_ADDRSTRLEN);
	net_address_to_string(raw->af, (void*) &raw->internal_ip_data, address, INET6_ADDRSTRLEN + 1);
	return address;
}

int ip_convert_address(const char* text_address, int port, struct sockaddr* addr, socklen_t* addr_len)
{
	struct sockaddr_in6 addr6;
	struct sockaddr_in addr4;
	size_t sockaddr_size;
	const char* taddr = 0;

	int ipv6sup = net_is_ipv6_supported();

	if (strcmp(text_address, "any") == 0)
	{
		if (ipv6sup)
			taddr = "::";
		else
			taddr = "0.0.0.0";
	}
	else if (strcmp(text_address, "loopback") == 0)
	{
		if (ipv6sup)
			taddr = "::1";
		else
			taddr = "127.0.0.1";
	}
	else
	{
		taddr = text_address;
	}


	if (ip_is_valid_ipv6(taddr) && ipv6sup)
	{
		sockaddr_size = sizeof(struct sockaddr_in6);
		memset(&addr6, 0, sockaddr_size);
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(port);
		if (net_string_to_address(AF_INET6, taddr, &addr6.sin6_addr) <= 0)
		{
			LOG_ERROR("Unable to convert socket address (ipv6)");
			return -1;
		}

		memcpy(addr, &addr6, sockaddr_size);
		*addr_len = sockaddr_size;
	}
	else if (ip_is_valid_ipv4(taddr))
	{
		sockaddr_size = sizeof(struct sockaddr_in);
		memset(&addr4, 0, sockaddr_size);
		addr4.sin_family = AF_INET;
		addr4.sin_port = htons(port);
		if (net_string_to_address(AF_INET, taddr, &addr4.sin_addr) <= 0)
		{
			LOG_ERROR("Unable to convert socket address (ipv4)");
			return -1;
		}
		memcpy(addr, &addr4, sockaddr_size);
		*addr_len = sockaddr_size;
	}
	else
	{
		addr = 0;
		*addr_len = 0;
		return -1;
	}

	return 0;
}

int ip_mask_create_left(int af, int bits, struct ip_addr_encap* result)
{
	uint32_t mask;

	memset(result, 0, sizeof(struct ip_addr_encap));
	result->af = af;

	if (af == AF_INET)
	{
		if (bits <= 0)
			mask = 0;
		else if (bits >= 32)
			mask = 0xffffffffU;
		else
			mask = (0xffffffffU << (32 - bits));

		result->internal_ip_data.in.s_addr = htonl(mask);
	}
	else if (af == AF_INET6)
	{
		if (bits >= 128)
		{
			memset(&result->internal_ip_data.in6, 0xff, 16);
		}
		// The data is already 0 if bits <= 0
		else if (bits > 0)
		{
			int n = 0;
			uint8_t* addr6 = (uint8_t*) &result->internal_ip_data.in6;

			while (bits >= 8)
			{
				addr6[n++] = 0xff;
				bits -= 8;
			}

			if (bits > 0)
			{
				mask = (0xffU << (8 - bits));
				addr6[n] = (uint8_t) mask;
			}
		}
	}
	else
	{
		return -1;
	}

#ifdef IP_CALC_DEBUG
	char* r_str = hub_strdup(ip_convert_to_string(result));
	LOG_DUMP("Created left mask: %s", r_str);
	hub_free(r_str);
#endif

	return 0;
}


int ip_mask_create_right(int af, int bits, struct ip_addr_encap* result)
{
	uint32_t mask;

	memset(result, 0, sizeof(struct ip_addr_encap));
	result->af = af;

	if (af == AF_INET)
	{
		if (bits <= 0)
			mask = 0;
		else if (bits >= 32)
			mask = 0xffffffffU;
		else
			mask = (0xffffffffU >> (32 - bits));

		result->internal_ip_data.in.s_addr = htonl(mask);
	}
	else if (af == AF_INET6)
	{
		if (bits >= 128)
		{
			memset(&result->internal_ip_data.in6, 0xff, 16);
		}
		// The data is already 0 if bits <= 0
		else if (bits > 0)
		{
			int n = 15;
			uint8_t* addr6 = (uint8_t*) &result->internal_ip_data.in6;

			while (bits >= 8)
			{
				addr6[n--] = 0xff;
				bits -= 8;
			}

			if (bits > 0)
			{
				mask = (0xffU >> (8 - bits));
				addr6[n] = mask;
			}
		}
	}
	else
	{
		return -1;
	}

#ifdef IP_CALC_DEBUG
	char* r_str = hub_strdup(ip_convert_to_string(result));
	LOG_DUMP("Created right mask: %s", r_str);
	hub_free(r_str);
#endif

	return 0;
}


void ip_mask_apply_AND(struct ip_addr_encap* addr, struct ip_addr_encap* mask, struct ip_addr_encap* result)
{
	memset(result, 0, sizeof(struct ip_addr_encap));
	result->af = addr->af;

	if (addr->af == AF_INET)
	{
		result->internal_ip_data.in.s_addr = addr->internal_ip_data.in.s_addr & mask->internal_ip_data.in.s_addr;
	}
	else if (addr->af == AF_INET6)
	{
		uint8_t* addr6   = (uint8_t*) &addr->internal_ip_data.in6;
		uint8_t* mask6   = (uint8_t*) &mask->internal_ip_data.in6;
		uint8_t* result6 = (uint8_t*) &result->internal_ip_data.in6;

		int n;
		for (n = 0; n < 16; n++)
		{
			result6[n] = addr6[n] & mask6[n];
		}
	}
}

void ip_mask_apply_OR(struct ip_addr_encap* addr, struct ip_addr_encap* mask, struct ip_addr_encap* result)
{
	memset(result, 0, sizeof(struct ip_addr_encap));
	result->af = addr->af;

	if (addr->af == AF_INET)
	{
		result->internal_ip_data.in.s_addr = addr->internal_ip_data.in.s_addr | mask->internal_ip_data.in.s_addr;
	}
	else if (addr->af == AF_INET6)
	{
		uint8_t* addr6   = (uint8_t*) &addr->internal_ip_data.in6;
		uint8_t* mask6   = (uint8_t*) &mask->internal_ip_data.in6;
		uint8_t* result6 = (uint8_t*) &result->internal_ip_data.in6;

		int n;
		for (n = 0; n < 16; n++)
		{
			result6[n] = addr6[n] | mask6[n];
		}
	}
}

int ip_compare(struct ip_addr_encap* a, struct ip_addr_encap* b)
{
	int ret = 0;
	uint32_t A, B;

	uhub_assert(a->af == b->af);

	if (a->af == AF_INET)
	{
		A = ntohl(a->internal_ip_data.in.s_addr);
		B = ntohl(b->internal_ip_data.in.s_addr);
		ret = A - B;
	}
	else if (a->af == AF_INET6)
	{
		ret = memcmp(&a->internal_ip_data.in6, &b->internal_ip_data.in6, 16);
	}

#ifdef IP_CALC_DEBUG
	char* a_str = hub_strdup(ip_convert_to_string(a));
	char* b_str = hub_strdup(ip_convert_to_string(b));
	LOG_DUMP("Comparing IPs '%s' AND '%s' => %d", a_str, b_str, ret);
	hub_free(a_str);
	hub_free(b_str);
#endif

	return ret;
}

static int check_ip_mask(const char* text_addr, int bits, struct ip_range* range)
{
	if (ip_is_valid_ipv4(text_addr) || ip_is_valid_ipv6(text_addr))
	{
		struct ip_addr_encap addr;
		struct ip_addr_encap mask1;
		struct ip_addr_encap mask2;
		int af = ip_convert_to_binary(text_addr, &addr);        /* 192.168.1.2 */
		int maxbits = (af == AF_INET6 ? 128 : 32);
		bits = MIN(MAX(bits, 0), maxbits);
		(void)ip_mask_create_left(af, bits, &mask1);            /* 255.255.255.0 */
		(void)ip_mask_create_right(af, maxbits - bits, &mask2); /* 0.0.0.255 */
		ip_mask_apply_AND(&addr, &mask1, &range->lo);           /* 192.168.1.0 */
		ip_mask_apply_OR(&range->lo, &mask2, &range->hi);       /* 192.168.1.255 */
		return 1;
	}
	return 0;
}

static int check_ip_range(const char* lo, const char* hi, struct ip_range* range)
{
	int ret1, ret2;
	if ((ip_is_valid_ipv4(lo) && ip_is_valid_ipv4(hi)) || (ip_is_valid_ipv6(lo) && ip_is_valid_ipv6(hi)))
	{
		ret1 = ip_convert_to_binary(lo, &range->lo);
		ret2 = ip_convert_to_binary(hi, &range->hi);
		if (ret1 == -1 || ret2 == -1 || ret1 != ret2)
		{
			return 0;
		}
		return 1;
	}
	return 0;
}

int ip_convert_address_to_range(const char* address, struct ip_range* range)
{
	int ret = 0;
	char* addr = 0;
	const char* split;

	if (!address || !range)
		return 0;

	split = strrchr(address, '/');
	if (split)
	{
		int mask = uhub_atoi(split + 1);
		if (mask == 0 && split[1] != '0')
			return 0;
		addr = hub_strndup(address, split - address);
		ret = check_ip_mask(addr, mask, range);
		hub_free(addr);
		return ret;
	}

	split = strrchr(address, '-');
	if (split)
	{
		addr = hub_strndup(address, split - address);
		ret = check_ip_range(addr, split + 1, range);
		hub_free(addr);
		return ret;
	}

	if (ip_is_valid_ipv4(address) || ip_is_valid_ipv6(address))
	{
		if (ip_convert_to_binary(address, &range->lo) == -1)
			return 0;
		memcpy(&range->hi, &range->lo, sizeof(struct ip_addr_encap));
		return 1;
	}
	return 0;
}

int ip_in_range(struct ip_addr_encap* addr, struct ip_range* range)
{
	return (addr->af == range->lo.af && ip_compare(&range->lo, addr) <= 0 && ip_compare(addr, &range->hi) <= 0);
}
