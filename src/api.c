// -*- mode: c; tab-width: 4; indent-tabs-mode: 1; st-rulers: [70] -*-
// vim: ts=8 sw=8 ft=c noet
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2013 Pagoda Box, Inc.  All rights reserved.
 */

#include <bframe.h>
#include <msgxchng.h>
#include <stdio.h>			/* standard buffered input/output */
#include <stdlib.h>			/* standard library definitions */
#include <string.h>
#include <syslog.h>			/* definitions for system error logging */
#include <unistd.h>			/* standard symbolic constants and types */
#include <uv.h>

#include "api.h"
#include "log.h"
#include "vtepd.h"

#include "api/ip_add.h"
#include "api/ip_list.h"
#include "api/ip_remove.h"
#include "api/node_add.h"
#include "api/node_list.h"
#include "api/node_remove.h"
#include "api/ping.h"
#include "api/status.h"
#include "helper.h"

static api_client_t
*new_api_client(uv_stream_t *stream)
{
	api_client_t *client = (api_client_t *)malloc(sizeof(api_client_t));

	client->stream = stream;
	client->buf    = new_bframe_buffer();

	return client;
}

static void
on_write(uv_write_t* req, int status)
{
	uv_buf_t *buf = (uv_buf_t *)req->data;
	free(buf->base);
	buf->base = NULL;
	free(req->data);
	req->data = NULL;
	free(req);
	req = NULL;
}

char
*pack_reply_data(reply_data_t *return_data, int elements, int *size)
{
	msgpack_sbuffer *buffer = NULL;
	msgpack_packer *packer = NULL;
	char *return_char;

	buffer = msgpack_sbuffer_new();
	msgpack_sbuffer_init(buffer);

	packer = msgpack_packer_new((void *)buffer, msgpack_sbuffer_write);
	msgpack_pack_map(packer, elements);

	for (int i = 0; i < elements; i++) {
		msgpack_pack_key_value(packer, 
			return_data[i].key, 
			return_data[i].key_len, 
			return_data[i].value, 
			return_data[i].value_len);
	}

	return_char = (char *)malloc(buffer->size + 1);
	memcpy(return_char, &buffer->data[0], buffer->size);
	return_char[buffer->size] = '\0';
	*size = buffer->size;

	msgpack_packer_free(packer);
	msgpack_sbuffer_free(buffer);
	return return_char;
}

void
reply(api_client_t *client, msgxchng_response_t *res)
{
	int p_len;
	char *payload = pack_msgxchng_response(res, &p_len);

	bframe_t *frame;
	frame = new_bframe(payload, p_len);

	uv_buf_t *buf;
	uv_write_t *writer;

	buf    = (uv_buf_t *)malloc(sizeof(uv_buf_t));
	writer = (uv_write_t *)malloc(sizeof(uv_write_t));

	int size;
	buf->base = bframe_to_char(frame, &size);
	buf->len  = size;

	if (uv_write(writer, client->stream, buf, 1, on_write) == UV_OK)
		writer->data = (void *)buf;

	/* cleanup */
	free(payload);
	clean_msgxchng_response(res);
	free(res);
	res = NULL;

	clean_bframe(frame);
	free(frame);
	frame = NULL;
}

void
reply_error(api_client_t *client, msgxchng_request_t *req, char *error_message)
{
	msgxchng_response_t *res;
	int size;
	char *data;

	reply_data_t return_data[] = {
		{
			"return",
			6,
			"error",
			5
		},
		{
			"error",
			5,
			error_message,
			strlen(error_message)
		}
	};

	data = pack_reply_data(return_data, 2, &size);
	res = new_msgxchng_response(req->id, req->id_len, data, size, "complete", 8);
	
	reply(client, res);
	free(data);
	data = NULL;
}

void
reply_success(api_client_t *client, msgxchng_request_t *req)
{
	msgxchng_response_t *res;
	int size;
	char *data;

	reply_data_t return_data[] = {
		{
			"return",
			6,
			"success",
			7
		}
	};

	data = pack_reply_data(return_data, 1, &size);
	res  = new_msgxchng_response(req->id, req->id_len, data, size, "complete", 8);

	reply(client, res);
	free(data);
	data = NULL;
}

static void
handle_unknown_command(api_client_t *client, msgxchng_request_t *req)
{
	vtep_log(VTEPD_DEBUG, "unknown command: %s", req->command);
}

static void
parse_request(api_client_t *client, bframe_t *frame)
{
	msgxchng_request_t *req;

	req = unpack_msgxchng_request(frame->data, frame->len.int_len);

	if (!strcmp(req->command, "ping"))
		handle_ping(client, req);
	else if (!strcmp(req->command, "status"))
		handle_status(client, req);
	else if (!strcmp(req->command, "ip.add"))
		handle_ip_add(client, req);
	else if (!strcmp(req->command, "ip.list"))
		handle_ip_list(client, req);
	else if (!strcmp(req->command, "ip.remove"))
		handle_ip_remove(client, req);
	else if (!strcmp(req->command, "node.add"))
		handle_node_add(client, req);
	else if (!strcmp(req->command, "node.list"))
		handle_node_list(client, req);
	else if (!strcmp(req->command, "node.remove"))
		handle_node_remove(client, req);
	else
		handle_unknown_command(client, req);

	/* cleanup */
	clean_bframe(frame);
	free(frame);
	frame = NULL;
}

static uv_buf_t
read_alloc_buffer(uv_handle_t *handle, size_t suggested_size)
{
	return uv_buf_init((char *)malloc(suggested_size), suggested_size);
}

static void
on_read(uv_stream_t *proto, ssize_t nread, uv_buf_t buf)
{
	api_client_t *client = (api_client_t *)proto->data;

	if (nread < 0) {
		if (buf.base) {
			free(buf.base);
			buf.base = NULL;
		}
		free(client->buf);
		free(client);
		uv_close((uv_handle_t *)proto, (uv_close_cb) free);
		return;
	}

	if (nread == 0) {
		free(buf.base);
		buf.base = NULL;
		return;
	}

	int framec;
	bframe_t **frames;

	frames = parse_char_to_bframes(buf.base, nread, client->buf, &framec);

	for (int i=0; i < framec ; i++) 
		parse_request(client, frames[i]);

	/* cleanup */
	free(buf.base);
	buf.base = NULL;

	free(frames);
	frames = NULL;
}

static void
on_connect(uv_stream_t* listener, int status)
{
	if (status == -1) {
		vtep_log(VTEPD_WARNING, "connect attempt failed");
		return;
	}

	vtep_log(VTEPD_DEBUG, "connection established");

	uv_tcp_t *proto = (uv_tcp_t *) malloc(sizeof(uv_tcp_t));
	if (uv_tcp_init(server.loop, proto) == UV_OK) {

		api_client_t *client = new_api_client((uv_stream_t *) proto);
		proto->data = (void *)client;

		if (!((uv_accept(listener, client->stream) == UV_OK) &&
				(uv_read_start(client->stream, read_alloc_buffer, on_read) == UV_OK))) {
			vtep_log(VTEPD_WARNING, "WARNING: Need to free data here, memory leaks ahoy!");
		}
	}

}

static void
free_uv_tcp(uv_handle_t* handle)
{
	free((uv_tcp_t *)handle);
}

/* Initialize a set of file descriptors to listen to the specified 'port'
 * binding the addresses specified in the VTEP server configuration.
 *
 * The listening file descriptors are stored in the integer array 'fds'
 * and their number is set in '*count'.
 *
 * The addresses to bind are specified in the global server.bindaddr array
 * and their number is server.bindaddr_count. If the server configuration
 * contains no specific addresses to bind, this function will try to
 * bind * (all addresses) for both the IPv4 and IPv6 protocols.
 *
 * On success the function returns VTEPD_OK.
 *
 * On error the function returns VTEPD_ERR. For the function to be on
 * error, at least one of the server.bindaddr addresses was
 * impossible to bind, or no bind addresses were specified in the server
 * configuration but the function is not able to bind * for at least
 * one of the IPv4 or IPv6 protocols. */
static int
listen_to_port(int port, uv_tcp_t *fds[], int *count)
{
	int j;
	struct sockaddr_in addr;
	struct sockaddr_in6 addr6;

	/* Force binding of 0.0.0.0 if no bind address is specified, always
	* entering the loop if j == 0. */
	if (server.bindaddr_count == 0) server.bindaddr[0] = NULL;
	for (j = 0; j < server.bindaddr_count || j == 0; j++) {
		fds[*count] = NULL;
		if (server.bindaddr[j] == NULL) {
			/* Bind * for both IPv6 and IPv4, we enter here only if
			* server.bindaddr_count == 0. */
			addr6 = uv_ip6_addr("::", port);
			fds[*count] = malloc(sizeof(uv_tcp_t));
			if ((uv_tcp_init(server.loop, fds[*count]) == UV_OK) &&
					(uv_tcp_bind6(fds[*count], addr6) == UV_OK) &&
					(uv_listen((uv_stream_t *)(fds[*count]), 128, on_connect) == UV_OK)) {
				(*count)++;
			} else {
				(void) free(fds[*count]);
				fds[*count] = NULL;
			}
			addr = uv_ip4_addr("0.0.0.0", port);
			fds[*count] = malloc(sizeof(uv_tcp_t));
			if ((uv_tcp_init(server.loop, fds[*count]) == UV_OK) &&
					(uv_tcp_bind(fds[*count], addr) == UV_OK) &&
					(uv_listen((uv_stream_t *)(fds[*count]), 128, on_connect) == UV_OK)) {
				(*count)++;
			} else {
				uv_close((uv_handle_t *) fds[*count], free_uv_tcp);
				// (void) free(fds[*count]);
				fds[*count] = NULL;
			}
			/* Exit the loop if we were able to bind * on IPv4 or IPv6,
			 * otherwise fds[*count] will be NULL and we'll print an
			 * error and return to the caller with an error. */
			if (*count) break;
		} else if (strchr(server.bindaddr[j],':')) {
			/* Bind IPv6 address. */
			addr6 = uv_ip6_addr(server.bindaddr[j], port);
			fds[*count] = malloc(sizeof(uv_tcp_t));
			if (!((uv_tcp_init(server.loop, fds[*count]) == UV_OK) &&
					(uv_tcp_bind6(fds[*count], addr6) == UV_OK) &&
					(uv_listen((uv_stream_t *)(fds[*count]), 128, on_connect) == UV_OK))) {
				(void) free(fds[*count]);
				fds[*count] = NULL;
			}
		} else {
			/* Bind IPv4 address. */
			addr = uv_ip4_addr(server.bindaddr[j], port);
			fds[*count] = malloc(sizeof(uv_tcp_t));
			if (!((uv_tcp_init(server.loop, fds[*count]) == UV_OK) &&
					(uv_tcp_bind(fds[*count], addr) == UV_OK) &&
					(uv_listen((uv_stream_t *)(fds[*count]), 128, on_connect) == UV_OK))) {
				(void) free(fds[*count]);
				fds[*count] = NULL;
			}
		}
		if (fds[*count] == NULL) {
			vtep_log(VTEPD_WARNING,
				"Creating Server TCP listening socket %s:%d: %s (%s)",
				server.bindaddr[j] ? server.bindaddr[j] : "*",
				server.port, uv_strerror(uv_last_error(server.loop)),
				uv_err_name(uv_last_error(server.loop)));
			return VTEPD_ERR;
		}
		(*count)++;
	}
	return VTEPD_OK;
}

void
init_api(void)
{
	/* Open the TCP listening socket for the user commands. */
	if (listen_to_port(server.port, server.ipfd, &server.ipfd_count) == VTEPD_ERR)
		exit(1);

	/* Abort if there are no listening sockets at all. */
	if (server.ipfd_count == 0) {
		vtep_log(VTEPD_WARNING, "Configured to not listen anywhere, exiting.");
		exit(1);
	}
}

static void
close_ports()
{
	int i;
	for (i = 0; i < server.ipfd_count; i++) {
		uv_close((uv_handle_t *)server.ipfd[i], free_uv_tcp);
		server.ipfd[i] = NULL;
	}
}

void
shutdown_api()
{
	vtep_log(VTEPD_DEBUG, "Shutting down API");
	close_ports();
}
