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
#include "ds_stack.h"

#include <errno.h>

struct _ds_stack
{
	ds_list entries;
	int num_entries;
};

/* allocate and initialize the stack */
ds_stack *ds_stack_alloc(void *stack_data)
{
	struct _ds_stack *stack = NULL;

	stack = calloc(sizeof(*stack), 1);
	if (!stack)
		return NULL;
	ds_list_init(&stack->entries, stack);

	return stack;
}

/* free the stack */
void ds_stack_free(ds_stack *stack)
{
	DS_ASSERT(!stack);
	DS_ASSERT(stack->num_entries);
	ds_list_deinit(&stack->entries);
	free(stack);
}

/* push stack_data to the last */
int ds_stack_push(ds_stack *stack, void *stack_data)
{
	ds_list *list = NULL;

	DS_ASSERT(!stack);

	list = ds_list_alloc(stack_data);
	if (!list)
		return -ENOMEM;
	ds_list_insert_after(list, &stack->entries);
	stack->num_entries++;

	return;
}

/* pop the last stack */
void *ds_stack_pop(ds_stack *stack)
{
	ds_list *list = NULL;
	void *stack_data = NULL;

	DS_ASSERT(!stack);

	list = ds_list_prev(&stack->entries);
	DS_ASSERT(list == NULL);
	stack_data = ds_list_get_list_data(list);

	ds_list_remove(list);
	ds_list_free(list);

	stack->num_entries--;

	return stack_data;
}

int ds_stack_size(ds_stack *stack)
{
	DS_ASSERT(!stack);
	return stack->num_entries;
}
