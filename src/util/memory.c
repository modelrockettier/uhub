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

#ifdef MEMORY_DEBUG

#define REALTIME_MALLOC_TRACKING

#ifdef REALTIME_MALLOC_TRACKING
#define UHUB_MAX_ALLOCS 50000
struct malloc_info
{
	void* ptr;
	size_t size;
	void* stack1;
	void* stack2;
};

static int hub_alloc_count = 0;
static size_t hub_alloc_size  = 0;
static int hub_alloc_peak_count = 0;
static size_t hub_alloc_peak_size  = 0;
static size_t hub_alloc_oom = 0;

static struct malloc_info hub_allocs[UHUB_MAX_ALLOCS] = { { 0, },  };
static int    malloc_slot = -1; /* free slot (-1, no slot) */

void internal_debug_print_leaks()
{
	size_t n = 0;
	size_t leak = 0;
	size_t count = 0;
	LOG_MEMORY("--- exit (allocs: %d, size: %" PRIsz ") ---", hub_alloc_count, hub_alloc_size);

	for (; n < UHUB_MAX_ALLOCS; n++)
	{
		if (hub_allocs[n].ptr)
		{
			leak += hub_allocs[n].size;
			count++;
			LOG_MEMORY("leak %p size: %" PRIsz " (bt: %p %p)", hub_allocs[n].ptr, hub_allocs[n].size, hub_allocs[n].stack1, hub_allocs[n].stack2);
		}
	}

	LOG_MEMORY("--- done (allocs: %d, size: %" PRIsz ", peak: %d/%" PRIsz ", oom: %" PRIsz ") ---", count, leak, hub_alloc_peak_count, hub_alloc_peak_size, hub_alloc_oom);
}
#endif /* REALTIME_MALLOC_TRACKING */

void* internal_debug_mem_malloc(size_t size, const char* where)
{
	size_t n = 0;
	char* ptr = malloc(size);

#ifdef REALTIME_MALLOC_TRACKING

	/* Make sure the malloc info struct is initialized */
	if (!hub_alloc_count)
	{
		LOG_MEMORY("--- start ---");
		for (n = 0; n < UHUB_MAX_ALLOCS; n++)
		{
			hub_allocs[n].ptr    = 0;
			hub_allocs[n].size   = 0;
			hub_allocs[n].stack1 = 0;
			hub_allocs[n].stack2 = 0;
		}

		atexit(&internal_debug_print_leaks);
	}

	if (ptr)
	{
		if (malloc_slot != -1)
			n = (size_t) malloc_slot;
		else
			n = 0;

		for (; n < UHUB_MAX_ALLOCS; n++)
		{
			if (!hub_allocs[n].ptr)
			{
				hub_allocs[n].ptr    = ptr;
				hub_allocs[n].size   = size;
				hub_allocs[n].stack1 = __builtin_return_address(1);
				hub_allocs[n].stack2 = __builtin_return_address(2);

				hub_alloc_size += size;
				hub_alloc_count++;

				hub_alloc_peak_count = MAX(hub_alloc_count, hub_alloc_peak_count);
				hub_alloc_peak_size  = MAX(hub_alloc_size,  hub_alloc_peak_size);

				LOG_MEMORY("%s %p (%d bytes) (bt: %p %p) {allocs: %d, size: %" PRIsz "}", where, ptr, (int) size, hub_allocs[n].stack1, hub_allocs[n].stack2, hub_alloc_count, hub_alloc_size);
				break;
			}
		}
	}
	else
	{
		LOG_MEMORY("%s *** OOM for %d bytes", where, size);
		hub_alloc_oom++;
		return 0;
	}
#endif /* REALTIME_MALLOC_TRACKING */
	return ptr;
}

void internal_debug_mem_free(void* ptr)
{
#ifdef REALTIME_MALLOC_TRACKING
	size_t n = 0;
	void* stack1 = __builtin_return_address(1);
	void* stack2 = __builtin_return_address(2);

	if (!ptr) return;

	for (; n < UHUB_MAX_ALLOCS; n++)
	{
		if (hub_allocs[n].ptr == ptr)
		{
			hub_alloc_size -= hub_allocs[n].size;
			hub_alloc_count--;
			hub_allocs[n].ptr    = 0;
			hub_allocs[n].size   = 0;
			hub_allocs[n].stack1 = 0;
			hub_allocs[n].stack2 = 0;
			LOG_MEMORY("free %p (bt: %p %p) {allocs: %d, size: %" PRIsz "}", ptr, stack1, stack2, hub_alloc_count, hub_alloc_size);
			malloc_slot = n;
			free(ptr);
			return;
		}
	}

	malloc_slot = -1;
	abort();
	LOG_MEMORY("free %p *** NOT ALLOCATED *** (bt: %p %p)", ptr, stack1, stack2);
#else
	free(ptr);
#endif /* REALTIME_MALLOC_TRACKING */
}

char* debug_mem_strdup(const char* s)
{
	size_t size = strlen(s);
	char* ptr = internal_debug_mem_malloc(size+1, "strdup");
	if (ptr)
	{
		memcpy(ptr, s, size);
		ptr[size] = 0;
	}
	return ptr;
}

char* debug_mem_strndup(const char* s, size_t n)
{
	size_t size = MIN(strlen(s), n);
	char* ptr = internal_debug_mem_malloc(size+1, "strndup");
	if (ptr)
	{
		memcpy(ptr, s, size);
		ptr[size] = 0;
	}
	return ptr;
}

void* debug_mem_calloc(size_t num, size_t size)
{
	// check for multiplication overflow
	if (size != 0 && num > (SIZE_MAX / size))
		return NULL;

	void* ptr = internal_debug_mem_malloc(num * size, "calloc");
	return ptr;
}

void* debug_mem_malloc(size_t size)
{
	void* ptr = internal_debug_mem_malloc(size, "malloc");
	return ptr;
}

void debug_mem_free(void *ptr)
{
	internal_debug_mem_free(ptr);
}


#endif

void* hub_malloc_zero(size_t size)
{
	void* data = hub_calloc(1, size);
	return data;
}

#ifdef DEBUG_FUNCTION_TRACE
#define FTRACE_LOG "ftrace.log"
static FILE* functrace = 0;

void main_constructor() __attribute__ ((no_instrument_function, constructor));
void main_deconstructor() __attribute__ ((no_instrument_function, destructor));
void __cyg_profile_func_enter(void* frame, void* callsite) __attribute__ ((no_instrument_function));
void __cyg_profile_func_exit(void* frame, void* callsite) __attribute__ ((no_instrument_function));


void main_constructor()
{
	functrace = fopen(FTRACE_LOG, "w");
	if (functrace == NULL)
	{
		fprintf(stderr, "Cannot create function trace file: " FTRACE_LOG "\n");
		exit(-1);
	}
}


void main_deconstructor()
{
	fclose(functrace);
}


void __cyg_profile_func_enter(void* frame, void* callsite)
{
	if (functrace)
		fprintf(functrace, "E%p\n", frame);
}

void __cyg_profile_func_exit(void* frame, void* callsite)
{
	if (functrace)
		fprintf(functrace, "X%p\n", frame);
}

#endif /* DEBUG_FUNCTION_TRACE */


