// -*- mode: c; tab-width: 4; indent-tabs-mode: 1; st-rulers: [70] -*-
// vim: ts=8 sw=8 ft=c noet
/*
 * Copyright (c) 2015 Pagoda Box Inc
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public License, v.
 * 2.0. If a copy of the MPL was not distributed with this file, You can obtain one
 * at http://mozilla.org/MPL/2.0/.
 */

#ifndef REDD_ASYNC_IO_H
#define REDD_ASYNC_IO_H

#include <sys/types.h>
#include <sys/socket.h>
#include <uv.h>

typedef struct async_io_buf_s async_io_buf_t;
typedef struct async_io_queue_s async_io_queue_t;
typedef struct async_io_s async_io_t;
typedef struct worker_thread_s worker_thread_t;
typedef struct async_io_baton_s async_io_baton_t;

typedef void (*async_io_cb)(void* data, int status);
typedef void (*async_io_done)(void* data, async_io_buf_t* elem);
typedef int (*async_io_each)(void* data, async_io_buf_t* elem);

struct async_io_baton_s {
	uv_work_t			req;
	async_io_queue_t	*async_io_queue;
	async_io_t 			*async_io;
	int 				events;
};

struct async_io_buf_s {
	ngx_queue_t 		queue;
	char 				*buf;
	int 				len;
	int 				maxlen;
	struct sockaddr_in  addr;
};

struct async_io_queue_s {
	int 				fd;
	void 				*data;
	int 				working;
	int 				flags;
	ngx_queue_t			avail_queue;
	ngx_queue_t			ready_queue;
	ngx_queue_t			work_queue;
	ngx_queue_t			work_done;
	int 				buf_len;
	int 				buf_count;
	async_io_done		_done;
	async_io_each		_each;
	async_io_cb			_cb;
	async_io_baton_t    baton;
	/* Worker thread stuff */
	uv_async_t			ping;
	uv_async_t			pong;
	ngx_queue_t			send_queue;
	int					signaled;
};

struct worker_thread_s {
	uv_thread_t me;
	uv_loop_t 	*loop;
};

struct async_io_s {
	uv_poll_t			poll;
	int 				fd;
	worker_thread_t		worker;
	async_io_queue_t    read_io;
	async_io_queue_t    write_io;
};

async_io_buf_t *async_io_buf_create(int len);
async_io_buf_t *async_io_write_buf_get(async_io_t *async_io);
int async_io_init(async_io_t *async_io, int fd, void *data,
	int read_buf_len, int read_buf_count, 
	async_io_each read_each, async_io_done read_done, async_io_cb read_cb,
	int write_buf_len, int write_buf_count, 
	async_io_each write_each, async_io_done write_done, async_io_cb write_cb);
int async_io_worker_init(async_io_t *async_io, int fd, void *data,
	int read_buf_len, int read_buf_count, 
	async_io_each read_each, async_io_done read_done, async_io_cb read_cb,
	int write_buf_len, int write_buf_count, 
	async_io_each write_each, async_io_done write_done, async_io_cb write_cb);
int async_io_poll_start(async_io_t *async_io);
int async_io_poll_stop(async_io_t *async_io);
void send_write_buffers(async_io_queue_t *queue);
void async_io_shutdown(async_io_t *async_io);

#endif
