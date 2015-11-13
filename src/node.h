// -*- mode: c; tab-width: 4; indent-tabs-mode: 1; st-rulers: [70] -*-
// vim: ts=8 sw=8 ft=c noet
/*
 * Copyright (c) 2015 Pagoda Box Inc
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public License, v.
 * 2.0. If a copy of the MPL was not distributed with this file, You can obtain one
 * at http://mozilla.org/MPL/2.0/.
 */

#ifndef REDD_NODE_H
#define REDD_NODE_H

#include <netinet/in.h>
#include <msgpack.h>

typedef struct red_node_s {
	char 				*hostname;
	struct sockaddr_in	send_addr;
} red_node_t;

void		*dup_node(void *ptr);
void		free_node(void *ptr);
int			match_node(void *ptr, void *key);

void		pack_node(msgpack_packer *packer, red_node_t *node);
void		pack_nodes(msgpack_packer *packer);
red_node_t	*unpack_node(msgpack_object object);
void		unpack_nodes(msgpack_object object);
int			validate_node(red_node_t *node);

void		save_nodes();
void		load_nodes();

void		add_red_node(red_node_t *node);
void		remove_red_node(red_node_t *key);

#endif
