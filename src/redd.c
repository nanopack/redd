// -*- mode: c; tab-width: 4; indent-tabs-mode: 1; st-rulers: [70] -*-
// vim: ts=8 sw=8 ft=c noet
/*
 * Copyright (c) 2015 Pagoda Box Inc
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public License, v.
 * 2.0. If a copy of the MPL was not distributed with this file, You can obtain one
 * at http://mozilla.org/MPL/2.0/.
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
#include "redd.h"
#include "log.h"
#include "node.h"
#include "ip.h"
#include "util/util.h"
#include "route.h"

/* server global state */
red_server_t server; 

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

	/* Every output goes to /dev/null. If RED is daemonized but
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
	printf("RED server v%s\n", REDD_VERSION);
	exit(0);
}

static void
usage(void)
{
	fprintf(stderr,"Usage: red [/path/to/red.conf] [options]\n");
	fprintf(stderr,"       red - (read config from stdin)\n");
	fprintf(stderr,"       red -v or --version\n");
	fprintf(stderr,"       red -h or --help\n\n");
	fprintf(stderr,"Examples:\n");
	fprintf(stderr,"       red (run the server with default conf)\n");
	fprintf(stderr,"       red /etc/red/4000.conf\n");
	fprintf(stderr,"       red --port 7777\n");
	fprintf(stderr,"       red /etc/myred.conf --loglevel verbose\n\n");
	exit(1);
}

static void
red_set_proc_title(char *title)
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
	red_log(REDD_DEBUG, "Caught a signal");
	uv_signal_stop(handle);
	shutdown_api();

	if (server.routing_enabled)
		shutdown_routing();

	// shutdown_server();

	clean_server_config();
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
		red_log(REDD_WARNING, "Warning: no config file specified, using the default config. In order to specify a config file use %s /path/to/red.conf", argv[0]);
	}
	if (server.daemonize) daemonize();
	init_server();
	if (server.routing_enabled)
		init_routing();
	init_api();
	if (server.daemonize) create_pidfile();
	red_set_proc_title(argv[0]);

	load_ips();
	load_nodes();

	red_log(REDD_WARNING, "Server started, RED version " REDD_VERSION);

	if (server.ipfd_count > 0)
		red_log(REDD_NOTICE, "The server is now ready to accept connections on port %d", server.port);

	uv_run(server.loop, UV_RUN_DEFAULT);

	uv_loop_delete(server.loop);

	return 0;
}
