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
#include <dirent.h>		/* open directory, get listings */
#include <locale.h>		/* set program locale */
#include <stdio.h>		/* standard buffered input/output */
#include <stdlib.h>		/* standard library definitions */
#include <syslog.h>		/* definitions for system error logging */
#include <sys/time.h>	/* time types */
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>		/* standard symbolic constants and types */

#include "api.h"
#include "config.h"
#include "vtepd.h"
#include "log.h"
#include "util/util.h"
#include "route.h"

/* server global state */
vtep_server_t server; 

static char
*human_readable(double value,int in_bits)
{
	if(value == 0 && in_bits)
		return "0 bytes";
	if(value == 0)
		return "0 bits";

	char tmp[22];
	char *modifier = " KMGTPEZY";
	int index = 0;
	while(value > 1024 && index < 8){
		value = value / 1024;
		index++;

	}
	if(in_bits){
		if(index == 0 )
			sprintf(tmp, "%.0f bytes", value);
		else
			sprintf(tmp, "%.1f %cB", value, modifier[index]);
	}else{
		if(index == 0 )
			sprintf(tmp, "%.0f bits", value);
		else
			sprintf(tmp, "%.1f %cb", value, modifier[index]);
	}
	return strdup(tmp);
}

static void
create_pidfile(void)
{
	/* Try to write the pid file in a best-effort way. */
	FILE *fp = fopen(server.pidfile,"w");
	if (fp) {
		fprintf(fp,"%d\n",(int)getpid());
		fclose(fp);
	}
}

static void
daemonize(void)
{
	int fd;

	if (fork() != 0) exit(0); /* parent exits */
	setsid(); /* create a new session */

	/* Every output goes to /dev/null. If VTEP is daemonized but
	* the 'logfile' is set to 'stdout' in the configuration file
	* it will not log at all. */
	if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		if (fd > STDERR_FILENO) close(fd);
	}
}

static void
version(void)
{
	printf("VTEP server v%s\n", VTEPD_VERSION);
	exit(0);
}

static void
usage(void)
{
	fprintf(stderr,"Usage: vtep [/path/to/vtep.conf] [options]\n");
	fprintf(stderr,"       vtep - (read config from stdin)\n");
	fprintf(stderr,"       vtep -v or --version\n");
	fprintf(stderr,"       vtep -h or --help\n\n");
	fprintf(stderr,"Examples:\n");
	fprintf(stderr,"       vtep (run the server with default conf)\n");
	fprintf(stderr,"       vtep /etc/vtep/4000.conf\n");
	fprintf(stderr,"       vtep --port 7777\n");
	fprintf(stderr,"       vtep /etc/myvtep.conf --loglevel verbose\n\n");
	exit(1);
}

static void
vtep_set_proc_title(char *title)
{
#ifdef USE_SETPROCTITLE
	setproctitle("%s %s:%d",
		title,
		server.bindaddr_count ? server.bindaddr[0] : "*",
		server.port);
#endif
}

static void
signal_handler(uv_signal_t* handle, int signum)
{
	uv_signal_stop(handle);
	uv_stop(server.loop);
}

static void
init_server(void)
{
	if (server.syslog_enabled) {
		openlog(server.syslog_ident, LOG_PID | LOG_NDELAY | LOG_NOWAIT, server.syslog_facility);
	}

	server.loop = uv_default_loop();
	uv_signal_init(server.loop, &server.signal_handle);
	uv_signal_start(&server.signal_handle, signal_handler, SIGUSR1);
	uv_signal_start(&server.signal_handle, signal_handler, SIGTERM);
	uv_signal_start(&server.signal_handle, signal_handler, SIGINT);
}

int
main(int argc, char **argv)
{
	struct timeval tv;

	setlocale(LC_COLLATE,"");
	srand(time(NULL)^getpid());
	gettimeofday(&tv,NULL);
	init_server_config();

	if (argc >= 2) {
		int j = 1; /* First option to parse in argv[] */
		sds options = sdsempty();
		char *configfile = NULL;

		/* Handle special options --help and --version */
		if (strcmp(argv[1], "-v") == 0 ||
			strcmp(argv[1], "--version") == 0) version();
		if (strcmp(argv[1], "--help") == 0 ||
			strcmp(argv[1], "-h") == 0) usage();

		/* First argument is the config file name? */
		if (argv[j][0] != '-' || argv[j][1] != '-')
			configfile = argv[j++];
		/* All the other options are parsed and conceptually appended to the
		* configuration file. For instance --port 6380 will generate the
		* string "port 6380\n" to be parsed after the actual file name
		* is parsed, if any. */
		while(j != argc) {
			if (argv[j][0] == '-' && argv[j][1] == '-') {
			/* Option name */
			if (sdslen(options)) options = sdscat(options,"\n");
				options = sdscat(options,argv[j]+2);
				options = sdscat(options," ");
			} else {
				/* Option argument */
				options = sdscatrepr(options,argv[j],strlen(argv[j]));
				options = sdscat(options," ");
			}
			j++;
		}
		load_server_config(configfile, options);
		sdsfree(options);
		if (configfile)
			server.configfile = getAbsolutePath(configfile);
	} else {
		vtep_log(VTEPD_WARNING, "Warning: no config file specified, using the default config. In order to specify a config file use %s /path/to/vtep.conf", argv[0]);
	}
	if (server.daemonize) daemonize();
	init_server();
	if (server.routing_enabled)
		routing_init();
	init_api();
	if (server.daemonize) create_pidfile();
	vtep_set_proc_title(argv[0]);

	vtep_log(VTEPD_WARNING, "Server started, VTEP version " VTEPD_VERSION);

	if (server.ipfd_count > 0)
		vtep_log(VTEPD_NOTICE, "The server is now ready to accept connections on port %d", server.port);

	uv_run(server.loop, UV_RUN_DEFAULT);

	shutdown_api();

	if (server.routing_enabled)
		shutdown_routing();

	shutdown_server();

	clean_server_config();

	return 0;
}
