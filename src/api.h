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

#ifndef VTEPD_API_H
#define VTEPD_API_H

#include <uv.h>
#include <bframe.h>
#include <msgxchng.h>

typedef struct {
	uv_stream_t 		*stream;
	bframe_buffer_t		*buf;
} api_client_t;

typedef struct reply_data_s {
	char 	*key;
	int 	key_len;
	char 	*value;
	int 	value_len;
} reply_data_t;

void	init_api(void);
void	reply(api_client_t *client, msgxchng_response_t *res);
char	*pack_reply_data(reply_data_t *return_data, int elements, int *size);
void	reply_error(api_client_t *client, msgxchng_request_t *req, char *error_message);
void	reply_success(api_client_t *client, msgxchng_request_t *req);

#endif
