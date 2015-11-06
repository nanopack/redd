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

#ifndef LOG_H
#define LOG_H

/* Log levels */
#define REDD_DEBUG				0
#define REDD_VERBOSE			1
#define REDD_NOTICE				2
#define REDD_WARNING			3
#define REDD_LOG_RAW			(1<<10)	/* Modifier to log without timestamp */
#define REDD_DEFAULT_VERBOSITY	REDD_NOTICE
#define REDD_MAX_LOGMSG_LEN		1024	/* Default maximum length of syslog messages */

#ifdef __GNUC__
void	red_log(int level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
#else
void	red_log(int level, const char *fmt, ...);
#endif
void	red_logRaw(int level, const char *msg);

#endif
