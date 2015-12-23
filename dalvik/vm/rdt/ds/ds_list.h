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

typedef struct _ds_list ds_list;
typedef struct _ds_list ds_list_head;

struct _ds_list
{
	void *list_data;			/* list_data */
	struct _ds_list *next;
	struct _ds_list *prev;
};

typedef void (*ds_list_func) (void *list_data, void *user_data);
typedef void *(*ds_list_func_stop) (void *list_data, void *user_data1, void *user_data2);
typedef void (*ds_list_compare_func) (const void *a, const void *b);

ds_list *ds_list_alloc(void *list_data);
void ds_list_free(ds_list *list);

void ds_list_init(ds_list *list, void *list_data);
void ds_list_deinit(ds_list *list);

void ds_list_insert_head(ds_list_head *head, ds_list *list);
void ds_list_insert_tail(ds_list_head *head, ds_list *list);
void ds_list_insert_after(ds_list *list, ds_list *at);
void ds_list_insert_before(ds_list *list, ds_list *at);
void ds_list_remove(ds_list *list);

ds_list *ds_list_prev(ds_list *list);
ds_list *ds_list_next(ds_list *list);

int ds_list_empty(ds_list_head *head);
ds_list *ds_list_first(ds_list_head *head);
ds_list *ds_list_last(ds_list_head *head);

ds_list *ds_list_reverse(ds_list *list);
ds_list *ds_list_find(ds_list_head *head, const void *list_data);
ds_list *ds_list_sort(ds_list *list, ds_list_compare_func compare_func);

void ds_list_iterate(ds_list *list, ds_list_func func, void *user_data);
void *ds_list_iterate_stop(ds_list *list, ds_list_func_stop func, void *user_data1, void *user_data2 ,int user_data3);

void *ds_list_get_list_data(ds_list *list);
