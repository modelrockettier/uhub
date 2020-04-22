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

struct linked_list* list_create()
{
	struct linked_list* list = NULL;
	list = (struct linked_list*) hub_malloc_zero(sizeof(struct linked_list));
	if (list == NULL)
		return NULL;
	return list;
}


void list_destroy(struct linked_list* list)
{
	if (list)
	{
		uhub_assert(list->size == 0);
		uhub_assert(list->first == NULL);
		uhub_assert(list->last == NULL);
		hub_free(list);
	}
}

void list_clear(struct linked_list* list, void (*free_handle)(void* ptr))
{
	struct node* node = list->first;
	struct node* tmp = NULL;
	while (node)
	{
		tmp = node->next;
		if (free_handle)
			free_handle(node->ptr);
		hub_free(node);
		node = tmp;
	}
	memset(list, 0, sizeof(struct linked_list));
}


void list_prepend(struct linked_list* list, void* data_ptr)
{
	struct node* new_node = (struct node*) hub_malloc_zero(sizeof(struct node));
	if (!new_node)
	{
		LOG_FATAL("Unable to allocate memory");
		return;
	}
	new_node->ptr = data_ptr;

	uhub_assert(list != NULL);
	// ensure either both list first and last are NULL or both are not NULL.
	uhub_assert((list->first == NULL) == (list->last == NULL));

	if (list->first)
	{
		new_node->next = list->first;
		list->first->prev = new_node;
	}
	else
	{
		list->last = new_node;
	}

	list->first = new_node;
	list->size++;
}

void list_append(struct linked_list* list, void* data_ptr)
{
	struct node* new_node = (struct node*) hub_malloc_zero(sizeof(struct node));
	if (!new_node)
	{
		LOG_FATAL("Unable to allocate memory");
		return;
	}
	new_node->ptr = data_ptr;

	uhub_assert(list != NULL);
	// ensure either both list first and last are NULL or both are not NULL.
	uhub_assert((list->first == NULL) == (list->last == NULL));

	if (list->last)
	{
		new_node->prev = list->last;
		list->last->next = new_node;
	}
	else
	{
		list->first = new_node;
	}

	list->last = new_node;
	list->size++;
}

void list_insert_ordered(struct linked_list* list, void* data_ptr, int (*cmp)(void*,void*))
{
	uhub_assert(list != NULL);
	uhub_assert(cmp != NULL);
	// ensure either both list first and last are NULL or both are not NULL.
	uhub_assert((list->first == NULL) == (list->last == NULL));

	if (!list->first || cmp(data_ptr, list->first->ptr) <= 0)
	{
		list_prepend(list, data_ptr);
	}
	else if (cmp(data_ptr, list->last->ptr) >= 0)
	{
		list_append(list, data_ptr);
	}
	else
	{
		// If we're here, then current is >= list->first
		struct node* current = list->first->next;
		struct node* new_node = (struct node*) hub_malloc_zero(sizeof(struct node));
		if (!new_node)
		{
			LOG_FATAL("Unable to allocate memory");
			return;
		}
		new_node->ptr = data_ptr;

		while (cmp(data_ptr, current->ptr) >= 0)
		{
			uhub_assert(current->next);
			current = current->next;
		}

		new_node->next = current;
		new_node->prev = current->prev;

		current->prev = new_node;
		new_node->prev->next = new_node;

		list->size++;
	}
}

void list_append_list(struct linked_list* list, struct linked_list* other)
{
	/* Anything to move? */
	if (!other->size)
		return;

	if (!list->size)
	{
		/* If the list is empty, just move the pointers */
		list->size = other->size;
		list->first = other->first;
		list->last = other->last;
		list->iterator = other->iterator;
	}
	else
	{
		other->first->prev = list->last;
		list->last->next = other->first;
		list->last = other->last;
		list->size += other->size;
	}

	/* Make sure the original list appears empty */
	other->size = 0;
	other->first = NULL;
	other->last = NULL;
	other->iterator = NULL;
}


void list_remove(struct linked_list* list, void* data_ptr)
{
	struct node* node;
	uhub_assert(list);
	uhub_assert(data_ptr);

	node = list->first;
	list->iterator = NULL;

	while (node)
	{
		if (node->ptr == data_ptr)
		{
			if (node->next)
				node->next->prev = node->prev;

			if (node->prev)
				node->prev->next = node->next;

			if (node == list->last)
				list->last = node->prev;

			if (node == list->first)
				list->first = node->next;

			hub_free(node);

			list->size--;
			return;
		}
		node = node->next;
	}
}

void list_remove_first(struct linked_list* list, void (*free_handle)(void* ptr))
{
	struct node* node = list->first;

	list->iterator = NULL;

	if (!node)
		return;

	list->first = node->next;

	if (list->first)
		list->first->prev = NULL;

	if (list->last == node)
		list->last = NULL;

	list->size--;

	if (free_handle)
		free_handle(node->ptr);
	hub_free(node);
}

void list_remove_last(struct linked_list* list, void (*free_handle)(void* ptr))
{
	struct node* node = list->last;

	list->iterator = NULL;

	if (!node)
		return;

	list->last = node->prev;

	if (list->last)
		list->last->next = NULL;

	if (list->first == node)
		list->first = NULL;

	list->size--;

	if (free_handle)
		free_handle(node->ptr);
	hub_free(node);
}


size_t list_size(struct linked_list* list)
{
	return list->size;
}


void* list_get_index(struct linked_list* list, size_t offset)
{
	struct node* node = list->first;
	size_t n = 0;
	for (n = 0; n < list->size; n++)
	{
		if (n == offset)
		{
			return node->ptr;
		}
		node = node->next;
	}
	return NULL;
}


void* list_get_first(struct linked_list* list)
{
	list->iterator = list->first;
	if (list->iterator == NULL)
		return NULL;

	return list->iterator->ptr;
}

struct node* list_get_first_node(struct linked_list* list)
{
	list->iterator = list->first;
	if (list->iterator == NULL)
		return NULL;

	return list->iterator;
}

void* list_get_last(struct linked_list* list)
{
	list->iterator = list->last;
	if (list->iterator == NULL)
		return NULL;

	return list->iterator->ptr;
}

struct node* list_get_last_node(struct linked_list* list)
{
	list->iterator = list->last;
	if (list->iterator == NULL)
		return NULL;

	return list->iterator;

}

void* list_get_next(struct linked_list* list)
{
	if (list->iterator == NULL)
		list->iterator = list->first;
	else
		list->iterator = list->iterator->next;

	if (list->iterator == NULL)
		return NULL;

	return list->iterator->ptr;
}


void* list_get_prev(struct linked_list* list)
{
	if (list->iterator == NULL)
		return NULL;

	list->iterator = list->iterator->prev;

	if (list->iterator == NULL)
		return NULL;

	return list->iterator->ptr;
}

