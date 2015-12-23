/*
 * datastructures - data structures for c programming
 *
 * Copyright (C) 2012-2014 Hyun Woo Kwon
 *
 * Author: Hyun Woo Kwon <starhwk@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "ds_common.h"
#include "ds_list.h"

/* alloc a list */
ds_list *ds_list_alloc(void *list_data)
{
	struct _ds_list *list;

	list = (_ds_list *) calloc(sizeof(*list), 1);
	if (!list)
		return NULL;

	list->list_data = list_data;

	list->next = list;
	list->prev = list;

	return list;
}

static void _ds_list_remove(ds_list *prev, ds_list *next)
{
	next->prev = prev;
	prev->next = next;
}

/* remove links and free a pointing list */
void ds_list_free(ds_list *list)
{
	DS_ASSERT(!list);
	_ds_list_remove(list->prev, list->next);
	free(list);
}

/* initialize an allocated list */
void ds_list_init(ds_list *list, void *list_data)
{
	DS_ASSERT(!list);
	DS_WARN(!list_data);
	list->list_data = list_data;
	list->next = list;
	list->prev = list;
}

/* deinitialize a list */
void ds_list_deinit(ds_list *list)
{
	DS_ASSERT(!list);
	_ds_list_remove(list->prev, list->next);
	list->list_data = NULL;
}

static void __ds_list_insert(ds_list *new_list, ds_list *prev, ds_list *next)
{
	next->prev = new_list;
	new_list->next = next;
	new_list->prev = prev;
	prev->next = new_list;
	return;
}

/* insert into the last of the head */
void ds_list_insert_head(ds_list_head *head, ds_list *list)
{
	DS_ASSERT(!head);
	DS_ASSERT(!list);
	__ds_list_insert(list, head, head->next);
}

/* insert into the last of the head */
void ds_list_insert_tail(ds_list_head *head, ds_list *list)
{
	DS_ASSERT(!head);
	DS_ASSERT(!list);
	__ds_list_insert(list, head->prev, head);
}

/* insert the list after at */
void ds_list_insert_after(ds_list *list, ds_list *at)
{
	DS_ASSERT(!at);
	DS_ASSERT(!list);
	__ds_list_insert(list, at, at->next);
}

/* insert the list before at */
void ds_list_insert_before(ds_list *list, ds_list *at)
{
	DS_ASSERT(!at);
	DS_ASSERT(!list);
	__ds_list_insert(list, at->prev, at);
}

/* remove links */
void ds_list_remove(ds_list *list)
{
	DS_ASSERT(!list);
	_ds_list_remove(list->prev, list->next);
}

/* get the previous list */
ds_list *ds_list_prev(ds_list *list)
{
	DS_ASSERT(!list);
	return list->prev;
}

/* get the next list */
ds_list *ds_list_next(ds_list *list)
{
	DS_ASSERT(!list);
	return list->next;
}

int ds_list_empty(ds_list_head *head)
{
	DS_ASSERT(!head);
	return (head->next == head);
}

ds_list *ds_list_first(ds_list_head *head)
{
	DS_ASSERT(!head);
	return ds_list_next(head);
}

ds_list *ds_list_last(ds_list_head *head)
{
	DS_ASSERT(!head);
	return ds_list_prev(head);
}

/* reverse the list */
ds_list *ds_list_reverse(ds_list *list)
{
	struct _ds_list *last = NULL;

	while (list) {
		last = list;
		list = last->next;
		last->next = last->prev;
		last->prev = list;
	}

	return last;
}

/* sort the list */
ds_list *ds_list_sort(ds_list *list, ds_list_compare_func compare_func)
{
	/* void */
	return NULL;
}

/* find the list with list_data */
ds_list *ds_list_find(ds_list_head *head, const void *list_data)
{
	struct _ds_list *list, *pos;

	DS_ASSERT(!head);

	list = head->next;
	while (list && (list != head)) {
		pos = list->next;
		if (list_data == list->list_data)
			break;
		list = pos;
	}

	if (list == head)
		list = NULL;

	return list;
}

/* iterate the list and call func */
void ds_list_iterate(ds_list_head *head, ds_list_func func, void *user_data)
{
	ds_list *current;

	DS_ASSERT(!head);

	current = head->next;
	while (current != head)
	{
		(*func)(current->list_data, user_data);
		current = current->next;
	}
}

/* DYLEE: iterate the list and call func - stop when return non-zero */
void *ds_list_iterate_stop(ds_list_head *head, ds_list_func_stop func, void *user_data1, void *user_data2, int user_data3)
{
	ds_list *current;
        void *stop = NULL;

	DS_ASSERT(!head);

	current = head->next;
	while (current != head)
	{
                stop = (*func)(current->list_data, user_data1, user_data2);
		if(stop) break;
		current = current->next;
	}
        return stop;
}

/* get the list_data from the list */
void *ds_list_get_list_data(ds_list *list)
{
	DS_ASSERT(!list);
	return list->list_data;
}
