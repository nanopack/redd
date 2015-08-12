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
#define VTEP_DEBUG				0
#define VTEP_VERBOSE			1
#define VTEP_NOTICE				2
#define VTEP_WARNING			3
#define VTEP_LOG_RAW			(1<<10)	/* Modifier to log without timestamp */
#define VTEP_DEFAULT_VERBOSITY	VTEP_NOTICE
#define VTEP_MAX_LOGMSG_LEN		1024	/* Default maximum length of syslog messages */

#ifdef __GNUC__
void	vtep_log(int level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
#else
void	vtep_log(int level, const char *fmt, ...);
#endif
void	vtep_logRaw(int level, const char *msg);

#endif