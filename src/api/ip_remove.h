// -*- mode: c; tab-width: 4; indent-tabs-mode: 1; st-rulers: [70] -*-
// vim: ts=8 sw=8 ft=c noet
/*
 * Copyright (c) 2015 Pagoda Box Inc
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public License, v.
 * 2.0. If a copy of the MPL was not distributed with this file, You can obtain one
 * at http://mozilla.org/MPL/2.0/.
 */

#ifndef REDD_API_IP_REMOVE_H
#define REDD_API_IP_REMOVE_H

#include <msgxchng.h>

#include "api.h"

void handle_ip_remove(api_client_t *client, msgxchng_request_t *req);

#endif
