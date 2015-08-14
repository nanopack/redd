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

#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "cmd.h"

int run_cmd(char* argv[])
{
	int status = -1;
	int ret = -1;
	pid_t child = 0;
	if ((child = fork()) == 0) {
		execvp(argv[0],argv);
		exit(1);
	} else if (child < 0) {
		return -1;
	}
	do {
		ret = waitpid(child, &status, 0);
	} while ( ret == -1 && errno == EINTR)

	return status;
}