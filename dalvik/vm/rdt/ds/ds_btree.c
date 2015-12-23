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

#include "ds_btree.h"
#include "ds_common.h"
#include "ds_queue.h"

#include <errno.h>

struct _ds_btree
{
	int key;
	void *btree_data;
	struct _ds_btree *left;
	struct _ds_btree *right;
};

struct _ds_btree **ds_btree_alloc(void)
{
	struct _ds_btree **btreep = calloc(sizeof(*btreep), 1);
	return btreep;
}

void ds_btree_free(struct _ds_btree **btreep)
{
	DS_ASSERT(btreep == NULL);
	free(btreep);
}

int ds_btree_add(struct _ds_btree **btreep, int key, void *btree_data)
{
	struct _ds_btree *btree = *btreep;
	int ret = 0;

#if 0 /* iterative */
	while (btree) {
		if (key == btree->key) {
			return ret;
		} else if (key < btree->key) {
			btreep = &(btree->left);
			btree = btree->left;
		} else if (key > btree->key) {
			btreep = &(btree->right);
			btree = btree->right;
		}
	}

	btree = calloc(sizeof(*btree), 1);
	if (!btree)
		return -ENOMEM;

	btree->key = key;
	btree->btree_data = btree_data;
	*btreep = btree;
#else /* recursive */
	if (!btree) {
		btree = calloc(sizeof(*btree), 1);
		if (!btree)
			return -ENOMEM;

		btree->key = key;
		btree->btree_data = btree_data;
		*btreep = btree;
	} else {
		if (key < btree->key)
			ret = ds_btree_add(&(btree->left), key, btree_data);
		else if (key > btree->key)
			ret = ds_btree_add(&(btree->right), key, btree_data);
	}
#endif

	return ret;
}

void *ds_btree_find(struct _ds_btree **btreep, int key)
{
	struct _ds_btree *btree = *btreep;
	void *btree_data;

	if (!btree)
		return NULL;

	if (key == btree->key)
		btree_data = btree->btree_data;
	else if (key < btree->key)
		btree_data = ds_btree_find(&(btree->left), key);
	else if (key > btree->key)
		btree_data = ds_btree_find(&(btree->right), key);

	return btree_data;
}

int ds_btree_df_iterate(struct _ds_btree **btreep, ds_btree_func func,
		void *user_data)
{
	struct _ds_btree *btree = *btreep;

	if (!btree)
		return 0;

	ds_btree_df_iterate(&(btree->left), func, user_data);
	func((void *)btree->key, user_data);
	ds_btree_df_iterate(&(btree->right), func, user_data);
}

int ds_btree_bf_iterate(struct _ds_btree **btreep, ds_btree_func func,
		void *user_data)
{
	struct _ds_btree *btree = *btreep;
	ds_queue *queue;

	if (!btree)
		return 0;

	queue = ds_queue_alloc();
	if (!queue)
		return -ENOMEM;

	ds_queue_enqueue(queue, btree);

	while (!ds_queue_empty(queue)) {
		btree = ds_queue_dequeue(queue);
		func((void *)btree->key, user_data);
		if (btree->left)
			ds_queue_enqueue(queue, btree->left);
		if (btree->right)
			ds_queue_enqueue(queue, btree->right);
	}

	ds_queue_free(queue);

	return 0;
}

void ds_btree_remove(struct _ds_btree **btreep, int key)
{
	struct _ds_btree *btree = *btreep;

	if (btree == NULL)
		goto finish;

	if (key == btree->key) {
		free(btree);
		*btreep = btree->left ? btree->left : btree->right;
	} else if (key < btree->key) {
		ds_btree_remove(&(btree->left), key);
	} else if (key > btree->key) {
		ds_btree_remove(&(btree->right), key);
	}

finish:
	return;
}
