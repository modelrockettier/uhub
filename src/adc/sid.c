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

// Used to convert binary to uppercase ascii base32
const char* BASE32_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

// Used to convert uppercase ascii base32 to binary
const int8_t BASE32_VALUES[128] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0x00
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0x10
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0x20
	-1, -1, 26, 27, 28, 29, 30, 31, -1, -1, -1, -1, -1, -1, -1, -1, // 0x30
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, // 0x40
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, // 0x50
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0x60
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0x70
};


char* sid_to_string(sid_t sid)
{
    static char t_sid[5];

    sid_t A;
	sid_t B;
	sid_t C;
	sid_t D;

	D    = (sid % 32);
	sid /= 32;
	C    = (sid % 32);
	sid /= 32;
	B    = (sid % 32);
	sid /= 32;
	A    = (sid % 32);
	sid /= 32;

	// sid too large
	uhub_assert(sid == 0);

    t_sid[0] = BASE32_ALPHABET[A];
    t_sid[1] = BASE32_ALPHABET[B];
    t_sid[2] = BASE32_ALPHABET[C];
    t_sid[3] = BASE32_ALPHABET[D];
    t_sid[4] = '\0';

	return t_sid;
}


sid_t string_to_sid(const char* sid)
{
    sid_t nsid = 0;
    sid_t n;
	sid_t x;
	uint8_t c;

    if (!sid || strlen(sid) != 4)
		return 0;

    for (n = 0; n < 4; n++) {
		c = sid[n];
		if (c >= 128)
			return 0;

		x = (sid_t) BASE32_VALUES[c];
        if (x == ((sid_t) -1))
			return 0;

        nsid = (nsid * 32) + x;
    }

    return nsid;
}

/*
 * Session IDs are heavily reused, since they are a fairly scarce
 * resource. Only one (2^10)-1 exist, since it is a four byte base32-encoded
 * value and 'AAAA' (0) is reserved for the hub.
 *
 * Initialize with sid_initialize(), which sets min and max to one, and count to 0.
 *
 * When allocating a session ID:
 * - If 'count' is less than the pool size (max-min), then allocate within the pool
 * - Increase the pool size (see below)
 * - If unable to do that, hub is really full - don't let anyone in!
 *
 * When freeing a session ID:
 * - If the session ID being freed is 'max', then decrease the pool size by one.
 *
 */

struct sid_pool
{
	sid_t min;
	sid_t max;
	sid_t count;
	struct hub_user** map;
};


struct sid_pool* sid_pool_create(sid_t max)
{
	struct sid_pool* pool = hub_malloc(sizeof(struct sid_pool));
	if (!pool)
		return 0;

	pool->min = 1;
	pool->max = max + 1;
	pool->count = 0;
	pool->map = hub_calloc(pool->max, sizeof(struct hub_user*));
	if (!pool->map)
	{
		hub_free(pool);
		return 0;
	}
	pool->map[0] = (struct hub_user*) pool; /* hack to reserve the first sid. */

#ifdef DEBUG_SID
	LOG_DUMP("SID_POOL:  max=%d", (int) pool->max);
#endif
	return pool;
}

void sid_pool_destroy(struct sid_pool* pool)
{
#ifdef DEBUG_SID
	LOG_DUMP("SID_POOL:  destroying, current allocs=%d", (int) pool->count);
#endif
	hub_free(pool->map);
	hub_free(pool);
}

sid_t sid_alloc(struct sid_pool* pool, struct hub_user* user)
{
	sid_t n;
	if (pool->count >= (pool->max - pool->min))
	{
#ifdef DEBUG_SID
		LOG_DUMP("SID_POOL:  alloc, sid pool is full.");
#endif
		return 0;
	}

	n = (++pool->count);
	for (; (pool->map[n % pool->max]); n++) ;

#ifdef DEBUG_SID
	LOG_DUMP("SID_ALLOC: %d, user=%p", (int) n, user);
#endif
	pool->map[n] = user;
	return n;
}

void sid_free(struct sid_pool* pool, sid_t sid)
{
#ifdef DEBUG_SID
	LOG_DUMP("SID_FREE:  %d", (int) sid);
#endif
	pool->map[sid] = 0;
	pool->count--;
}

struct hub_user* sid_lookup(struct sid_pool* pool, sid_t sid)
{
	if (!sid || (sid >= pool->max))
		return 0;
	return pool->map[sid];
}
