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
#include "ds_queue.h"
#include "ds_list.h"

#include <errno.h>

struct _ds_queue
{
	ds_list_head list_head;
	int size;
};

ds_queue *ds_queue_alloc(void)
{
	struct _ds_queue *queue;

	queue = (_ds_queue*) calloc(sizeof(*queue), 1);
	if (!queue)
		return NULL;

	ds_list_init(&queue->list_head, queue);

	return queue;
}

void ds_queue_free(ds_queue *queue)
{
	DS_ASSERT(!queue);
	DS_ASSERT(queue->size);

	ds_list_deinit(&queue->list_head);

	free(queue);
}

int ds_queue_enqueue(ds_queue *queue, void *entry)
{
	ds_list *list;

	DS_ASSERT(!queue);

	list = ds_list_alloc(entry);
	if (!list)
		return -ENOMEM;

	ds_list_insert_head(&queue->list_head, list);
	queue->size++;

	return 0;
}

void ds_queue_iterate(ds_queue *queue, ds_queue_func func, void *user_data)
{
	DS_ASSERT(!queue);
	ds_list_iterate(&queue->list_head, func, user_data);
}

void *ds_queue_iterate_stop(ds_queue *queue, ds_queue_func_stop func, void *user_data1, void *user_data2, int user_data3)
{
	DS_ASSERT(!queue);
	return ds_list_iterate_stop(&queue->list_head, func, user_data1, user_data2,user_data3);
}

void *ds_queue_dequeue(ds_queue *queue)
{
	ds_list *list;
	void *entry;

	DS_ASSERT(!queue);
	if (!queue->size)
		return NULL;

	list = ds_list_last(&queue->list_head);
	DS_ASSERT(!list);

	entry = ds_list_get_list_data(list);
	ds_list_free(list);

	queue->size--;

	return entry;
}

int ds_queue_empty(ds_queue *queue)
{
	DS_ASSERT(!queue);
	return (queue->size == 0);
}
