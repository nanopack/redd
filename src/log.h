// -*- mode: c; tab-width: 4; indent-tabs-mode: 1; st-rulers: [70] -*-
// vim: ts=8 sw=8 ft=c noet
/*
 * Copyright (c) 2015 Pagoda Box Inc
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public License, v.
 * 2.0. If a copy of the MPL was not distributed with this file, You can obtain one
 * at http://mozilla.org/MPL/2.0/.
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
