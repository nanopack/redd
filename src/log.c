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

#include "log.h"
#include "redd.h"

#include <stdio.h>		/* standard buffered input/output */
#include <stdlib.h>		/* standard library definitions */
#include <stdarg.h>		/* handle variable argument list */
#include <syslog.h>		/* definitions for system error logging */
#include <sys/time.h>	/* time types */
#include <unistd.h>		/* standard symbolic constants and types */

/* Low level logging. To use only for very big messages, otherwise
 * red_log() is to prefer. */
void
red_log_raw(int level, const char *msg)
{
	const int syslogLevelMap[] = { LOG_DEBUG, LOG_INFO, LOG_NOTICE, LOG_WARNING };
	const char *c = ".-*#";
	FILE *fp;
	char buf[64];
	int rawmode = (level & REDD_LOG_RAW);
	int log_to_stdout = (server.logfile == NULL || server.logfile[0] == '\0');

	level &= 0xff; /* clear flags */
	if (level < server.verbosity) return;

	fp = log_to_stdout ? stdout : fopen(server.logfile,"a");
	if (!fp) return;

	if (rawmode) {
		fprintf(fp,"%s",msg);
	} else {
		int off;
		struct timeval tv;

		gettimeofday(&tv,NULL);
		off = strftime(buf,sizeof(buf),"%d %b %H:%M:%S.",localtime(&tv.tv_sec));
		snprintf(buf+off,sizeof(buf)-off,"%03d",(int)tv.tv_usec/1000);
		fprintf(fp,"[%d] %s %c %s\n",(int)getpid(),buf,c[level],msg);
	}
	fflush(fp);

	if (!log_to_stdout) fclose(fp);
	if (server.syslog_enabled) syslog(syslogLevelMap[level], "%s", msg);
}

/* Like red_log_raw() but with printf-alike support. This is the function that
 * is used across the code. The raw version is only used in order to dump
 * the INFO output on crash. */
void
red_log(int level, const char *fmt, ...)
{
	va_list ap;
	char msg[REDD_MAX_LOGMSG_LEN];

	if ((level&0xff) < server.verbosity) return;

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	red_log_raw(level,msg);
}
