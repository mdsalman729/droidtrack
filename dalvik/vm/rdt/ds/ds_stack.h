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

typedef struct _ds_stack ds_stack;

ds_stack *ds_stack_alloc(void *stack_data);
void ds_stack_free(ds_stack *stack);

void ds_stack_init(ds_stack *stack);
void ds_stack_deinit(ds_stack *stack);

int ds_stack_push(ds_stack *stack, void *stack_data);
void *ds_stack_pop(ds_stack *stack);

int ds_stack_size(ds_stack *stack);
