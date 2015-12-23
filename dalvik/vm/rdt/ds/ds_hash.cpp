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
#include "ds_hash.h"
#include "ds_list.h"

#include <errno.h>
#include <cutils/log.h>

typedef struct _ds_hash_entry ds_hash_entry;

struct _ds_hash_entry
{
	ds_list *list;
	int key;
	void *hash_data;
};

struct _ds_hash
{
	int hash_bits;
	int hash_size;
	struct _ds_hash_entry **entry_heads;
};

/* allocate a hash table */
ds_hash *ds_hash_alloc(int hash_bits)
{
	struct _ds_hash *hash;
	int i;

	hash = (_ds_hash *) calloc(sizeof(*hash), 1);
	if (!hash)
		return NULL;

	hash->hash_bits = (hash_bits >= 0) ? hash_bits : 8;
	hash->hash_size = 1 << hash->hash_bits;
	hash->entry_heads = (_ds_hash_entry**) calloc(sizeof(struct _ds_hash_entry *),
			hash->hash_size);
	if (!hash->entry_heads)
		goto err_out_hash;

	for (i = 0; i < hash->hash_size; i++) {
		struct _ds_hash_entry *entry_head;

		entry_head = (_ds_hash_entry*) calloc(sizeof(struct _ds_hash_entry), 1);
		if (!entry_head)
			goto err_out_entry;
		hash->entry_heads[i] = entry_head;
		entry_head->list = ds_list_alloc(entry_head);
		if (!entry_head->list)
			goto err_out_entry;
	}

	return hash;

err_out_entry:
	for (i = 0; i < hash->hash_size; i++)
		if (hash->entry_heads[i]) {
			if (hash->entry_heads[i]->list)
				ds_list_free(hash->entry_heads[i]->list);
			free(hash->entry_heads[i]);
		}
	free(hash->entry_heads);
err_out_hash:
	free(hash);

	return NULL;
}

/* free a hash table */
void ds_hash_free(ds_hash *hash)
{
	int i;

	DS_ASSERT(!hash);

	for (i = 0; i < hash->hash_size; i++) {
		DS_ASSERT(!hash->entry_heads[i]->list);
		ds_list_free(hash->entry_heads[i]->list);
		DS_ASSERT(!hash->entry_heads[i]);
		free(hash->entry_heads[i]);
	}

	DS_ASSERT(!hash->entry_heads);
	free(hash->entry_heads);

	free(hash);
}

/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define HASH_32(val, bits)	(val * 0x9e370001) >> (32 - bits)

/* add an entry with a key / data */
int ds_hash_add(ds_hash *hash, int key, void *hash_data)
{
	struct _ds_hash_entry *entry;

	ds_list *list_head;
	ds_list *list;

	DS_ASSERT(!hash);
	DS_WARN(ds_hash_find(hash, key) == hash_data);

	list_head = hash->entry_heads[HASH_32(key, hash->hash_bits)]->list;
	DS_ASSERT(!list_head);

	entry = (_ds_hash_entry*) calloc(sizeof(struct _ds_hash_entry), 1);
	if (!entry) {
		LOGD("CANNOT MALLOC HASH ENTRY\n");
		return -ENOMEM;
        }

	entry->key = key;
	entry->hash_data = hash_data;

	list = ds_list_alloc(entry);
	if (!list) {
		LOGD("CANNOT MALLOC LIST ENTRY\n");
		free(entry);
		return -ENOMEM;
	}
	entry->list = list;
	ds_list_insert_tail(list_head, list);

	return 0;
}

/* find an entry with the key */
void *ds_hash_find(ds_hash *hash, int key)
{
	struct _ds_hash_entry *entry;
	void *hash_data = NULL;

	ds_list *list_head;
	ds_list *list;

	list_head = hash->entry_heads[HASH_32(key, hash->hash_bits)]->list;
	DS_ASSERT(!list_head);

	list = ds_list_next(list_head);
	while (list && (list != list_head)) {
		entry = (_ds_hash_entry*) ds_list_get_list_data(list);
		DS_ASSERT(!entry);
		if (entry->key == key) {
			hash_data = entry->hash_data;
			break;
		}
		list = ds_list_next(list);
	}

	return hash_data;
}

void ds_hash_iterate(ds_hash *hash, ds_hash_func func, void *user_data)
{
	struct _ds_hash_entry *entry;

	ds_list *list_head;
	ds_list *list = NULL;

	int i = 0;

	for (i = 0; i < hash->hash_size; i++) {
		list_head = hash->entry_heads[i]->list;
		DS_ASSERT(!list_head);

		list = ds_list_next(list_head);
		while ((list) && (list != list_head)) {
			entry = (_ds_hash_entry*) ds_list_get_list_data(list);
			DS_ASSERT(!entry);
			func(entry->key, user_data);
			list = ds_list_next(list);
		}
	}

	return;
}

/* remove an entry with the key */
void ds_hash_remove(ds_hash *hash, int key)
{
	struct _ds_hash_entry *entry;

	ds_list *list_head;
	ds_list *list;

	int found = 0;

	list_head = hash->entry_heads[HASH_32(key, hash->hash_bits)]->list;
	DS_ASSERT(!list_head);

	list = ds_list_next(list_head);
	while (list && (list != list_head)) {
		entry = (_ds_hash_entry*) ds_list_get_list_data(list);
		DS_ASSERT(!entry);
		if (entry->key == key) {
			ds_list_free(entry->list);
			free(entry);
			found = 1;
			break;
		}
		list = ds_list_next(list);
	}

	DS_WARN(!found);

	return;
}
