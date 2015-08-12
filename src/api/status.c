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

#include <stdlib.h>
#include <msgxchng.h>

#include "api.h"
#include "api/status.h"
#include "helper.h"
#include "log.h"

void
handle_status(api_client_t *client, msgxchng_request_t *req)
{
	msgxchng_response_t *res;
	res = new_msgxchng_response(req->id, req->id_len, req->data, req->data_len, "complete", 8);

	vtep_log(VTEP_DEBUG, "request %s: status", req->id);

	reply(client, res);

	/* cleanup */
	clean_msgxchng_request(req);
	free(req);
	req = NULL;
}