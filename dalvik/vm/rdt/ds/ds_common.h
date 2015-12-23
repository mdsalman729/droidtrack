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

#include <stdio.h>
#include <stdlib.h>

#define CONTAINER_OF(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define _DS_PRINT(type, msg, ...) \
	printf("[DS_" type "]%s(%s)%d " msg "\n", \
	__FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#if 1 /*DEBUG*/

#define DS_PRINT(...) printf(__VA_ARGS__)
#define DS_LOG(...) _DS_PRINT("LOG", __VA_ARGS__)
#define DS_DEBUG(...) _DS_PRINT("DEBUG", __VA_ARGS__)

#define DS_WARN(cond) {if (cond) {_DS_PRINT("WARN", #cond);}}
#define DS_ASSERT(cond) {if (cond) {_DS_PRINT("ASSERT", #cond); abort();}}

#else /*DEBUG*/

#define DS_LOG(msg, ...)
#define DS_DEBUG(msg, ...)

#define DS_WARN(cond)
#define DS_ASSERT(cond, ...)

#endif /*DEBUG*/

