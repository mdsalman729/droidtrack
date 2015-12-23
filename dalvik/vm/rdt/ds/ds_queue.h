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

typedef struct _ds_queue ds_queue;

typedef void (*ds_queue_func) (void *queue_data, void *user_data);
typedef void *(*ds_queue_func_stop) (void *queue_data, void *user_data1, void *user_data2);

ds_queue *ds_queue_alloc(void);
void ds_queue_free(ds_queue *queue);

int ds_queue_enqueue(ds_queue *queue, void *entry);
void ds_queue_iterate(ds_queue *queue, ds_queue_func, void *user_data);
void *ds_queue_iterate_stop(ds_queue *queue, ds_queue_func_stop, void *user_data1, void *user_data2,int user_data3);
void *ds_queue_dequeue(ds_queue *queue);

int ds_queue_empty(ds_queue *queue);
