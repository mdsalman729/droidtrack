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

typedef struct _ds_btree ds_btree;

typedef int (*ds_btree_func) (void *btree_data, void *user_data);

ds_btree **ds_btree_alloc(void);
void ds_btree_free(ds_btree **btreep);

int ds_btree_add(ds_btree **btreep, int key, void *btree_data);
void *ds_btree_find(ds_btree **btreep, int key);
int ds_btree_df_iterate(ds_btree **btreep, ds_btree_func func,
		void *user_data);
int ds_btree_bf_iterate(struct _ds_btree **btreep, ds_btree_func func,
		void *user_data);
void ds_btree_remove(ds_btree **btreep, int key);

