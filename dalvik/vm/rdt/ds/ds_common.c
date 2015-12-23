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

#include <errno.h>

struct _ds_stopwatch_record
{
	int time;
	char *name[DS_STOPWATCH_NAME_LEN];
};

struct _ds_stopwatch
{
	struct timeval tv;
	int current_start;
	int current_stop;
	ds_list_head times_head;
	int total_lapse;
	int count;
};

struct _ds_stopwatch *ds_stopwatch_alloc(void)
{
	struct _ds_stopwatch *stopwatch = calloc(sizeof(*stopwatch), 1);

	if (!stopwatch)
		return NULL;

	ds_list_init(&stopwatch->times_head, stopwatch);

	return stopwatch;
}

void ds_stopwatch_free(ds_stopwatch *stopwatch)
{
	ds_list *list;

	DS_ASSERT(!stopwatch);

	list = &stopwatch->times_head;
	while (!ds_list_empty(&stopwatch->times_head)) {
		struct _ds_stopwatch_record *record;

		list = ds_list_next(list);
		ds_list_remove(list);
		record = ds_list_get_list_data(list);
		free(record);
		ds_list_free(list);
	}

	ds_list_deinit(&stopwatch->times_head);
	free(stopwatch);
}

void ds_stopwatch_start(ds_stopwatch *stopwatch)
{
	DS_ASSERT(!stopwatch);

	gettimeofday(&stopwatch->tv, NULL);
	stopwatch->current_start = stopwatch->tv.tv_sec * 1000000 +
		stopwatch->tv.tv_usec;
}

void ds_stopwatch_stop(ds_stopwatch *stopwatch)
{
	DS_ASSERT(!stopwatch);

	gettimeofday(&stopwatch->tv, NULL);
	stopwatch->current_stop = stopwatch->tv.tv_sec * 1000000 +
		stopwatch->tv.tv_usec;
}

int ds_stopwatch_record(ds_stopwatch *stopwatch, char *name)
{
	ds_list *list = NULL;
	struct _ds_stopwatch_record *record;
	int time = stopwatch->current_stop - stopwatch->current_start;

	DS_ASSERT(!stopwatch);

	DS_WARN(stopwatch->current_stop < stopwatch->current_start);

	record = calloc(sizeof(*record), 1);
	if (!record)
		return -ENOMEM;

	record->time = time;
	strcpy(record->name, name);
	list = ds_list_alloc(record);
	DS_ASSERT(!list);

	ds_list_insert_tail(&stopwatch->times_head, list);
	stopwatch->total_lapse = time;
	stopwatch->count++;

	return 0;
}

void ds_stopwatch_clear(ds_stopwatch *stopwatch)
{
	ds_list *list = NULL;

	DS_ASSERT(!stopwatch);

	list = &stopwatch->times_head;
	while (!ds_list_empty(&stopwatch->times_head)) {
		struct _ds_stopwatch_record *record;

		list = ds_list_next(list);
		ds_list_remove(list);
		record = ds_list_get_list_data(list);
		free(record);
		ds_list_free(list);
	}
	stopwatch->current_start = 0;
	stopwatch->current_stop = 0;
	stopwatch->total_lapse = 0;
	stopwatch->count = 0;
}

static void _ds_stopwatch_util_print(void *data, void *user_data)
{
	int time = (int)data;
	int sec = 0;
	int msec = 0;
	int usec = 0;

	sec = time / 1000000;
	msec = time / 1000 - (time / 1000000) * 1000;
	usec = time - (time / 1000) * 1000 - (time / 1000000) * 1000;

	DS_PRINT("%d s %d ms %d us\n", sec, msec, usec);
}

void ds_stopwatch_show_lapse(ds_stopwatch *stopwatch)
{
	DS_ASSERT(!stopwatch);
	DS_WARN(stopwatch->current_stop < stopwatch->current_start);

	DS_PRINT("lapse : ");
	_ds_stopwatch_util_print((void *)(stopwatch->current_stop -
				stopwatch->current_start), NULL);
}

void ds_stopwatch_show_current(ds_stopwatch *stopwatch)
{
	struct timeval tv;

	DS_ASSERT(!stopwatch);
	gettimeofday(&tv, NULL);
	DS_PRINT("current : ");
	_ds_stopwatch_util_print((void *)(tv.tv_sec * 1000000 + tv.tv_usec -
				stopwatch->current_start), NULL);
}

void ds_stopwatch_show_average(ds_stopwatch *stopwatch)
{
	int time = 0;
	int sec = 0;
	int msec = 0;
	int usec = 0;

	DS_ASSERT(!stopwatch);
	DS_ASSERT(!stopwatch->count);

	time = stopwatch->total_lapse / stopwatch->count;
	sec = time / 1000000;
	msec = time / 1000 - (time / 1000000) * 1000;
	usec = time - (time / 1000) * 1000 - (time / 1000000) * 1000;

	DS_PRINT("average : %d s %d ms %d us\n", sec, msec, usec);
}

static void _ds_stopwatch_util_print_list(void *data, void *user_data)
{
	struct _ds_stopwatch_record *record = data;
	int time = record->time;
	int sec, msec, usec;

	sec = time / 1000000;
	msec = time / 1000 - (time / 1000000) * 1000;
	usec = time - (time / 1000) * 1000 - (time / 1000000) * 1000;

	DS_PRINT("[%s] %d s %d ms %d us\n", record->name, sec, msec, usec);
}

void ds_stopwatch_show_history(ds_stopwatch *stopwatch)
{
	ds_list *list = NULL;

	DS_ASSERT(!stopwatch);

	list = &stopwatch->times_head;
	DS_PRINT("-stopwatch history start-\n");
	ds_list_iterate(&stopwatch->times_head, _ds_stopwatch_util_print_list,
			NULL);
	DS_PRINT("-stopwatch history end-\n");
}
