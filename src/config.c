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

#include <errno.h>	/* system error numbers */
#include <stdio.h>	/* standard buffered input/output */
#include <stdlib.h>	/* standard library definitions */
#include <string.h>	/* string operations */
#include <syslog.h>	/* definitions for system error logging */

#include "ip.h"
#include "node.h"
#include "redd.h"
#include "vxlan.h"
#include "log.h"
#include "util/sds.h"		/* dynamic safe strings */

static struct {
	const char     *name;
	const int       value;
} validSyslogFacilities[] = {
	{"user",    LOG_USER},
	{"local0",  LOG_LOCAL0},
	{"local1",  LOG_LOCAL1},
	{"local2",  LOG_LOCAL2},
	{"local3",  LOG_LOCAL3},
	{"local4",  LOG_LOCAL4},
	{"local5",  LOG_LOCAL5},
	{"local6",  LOG_LOCAL6},
	{"local7",  LOG_LOCAL7},
	{NULL, 0}
};

static int
yesnotoi(char *s)
{
	if (!strcasecmp(s,"yes")) return 1;
	else if (!strcasecmp(s,"no")) return 0;
	else return -1;
}

static void
load_server_config_from_string(char *config)
{
	char *err = NULL;
	int linenum = 0, totlines, i;
	sds *lines;

	lines = sdssplitlen(config,strlen(config),"\n",1,&totlines);

	for (i = 0; i < totlines; i++) {
		sds *argv;
		int argc;

		linenum = i+1;
		lines[i] = sdstrim(lines[i]," \t\r\n");

		/* Skip comments and blank lines */
		if (lines[i][0] == '#' || lines[i][0] == '\0') continue;

		/* Split into arguments */
		argv = sdssplitargs(lines[i],&argc);
		if (argv == NULL) {
			err = "Unbalanced quotes in configuration line";
			goto loaderr;
		}

		/* Skip this line if the resulting command vector is empty. */
		if (argc == 0) {
			sdsfreesplitres(argv,argc);
			continue;
		}
		sdstolower(argv[0]);

		/* Execute config directives */
		if (!strcasecmp(argv[0],"pidfile") && argc == 2) {
			free(server.pidfile);
			server.pidfile = strdup(argv[1]);
		} else if (!strcasecmp(argv[0],"daemonize") && argc == 2) {
			if ((server.daemonize = yesnotoi(argv[1])) == -1) {
				err = "argument must be 'yes' or 'no'"; goto loaderr;
			}
		} else if (!strcasecmp(argv[0],"loglevel") && argc == 2) {
			if (!strcasecmp(argv[1],"debug")) server.verbosity = REDD_DEBUG;
			else if (!strcasecmp(argv[1],"verbose")) server.verbosity = REDD_VERBOSE;
			else if (!strcasecmp(argv[1],"notice")) server.verbosity = REDD_NOTICE;
			else if (!strcasecmp(argv[1],"warning")) server.verbosity = REDD_WARNING;
			else {
				err = "Invalid log level. Must be one of debug, notice, warning";
				goto loaderr;
			}
		} else if (!strcasecmp(argv[0],"logfile") && argc == 2) {
			FILE *logfp;

			free(server.logfile);
			server.logfile = strdup(argv[1]);
			if (server.logfile[0] != '\0') {
				/* Test if we are able to open the file. The server will not
				* be able to abort just for this problem later... */
				logfp = fopen(server.logfile,"a");
				if (logfp == NULL) {
					err = sdscatprintf(sdsempty(),
						"Can't open the log file: %s", strerror(errno));
					goto loaderr;
				}
				fclose(logfp);
			}
		} else if (!strcasecmp(argv[0],"syslog-enabled") && argc == 2) {
			if ((server.syslog_enabled = yesnotoi(argv[1])) == -1) {
				err = "argument must be 'yes' or 'no'"; goto loaderr;
			}
		} else if (!strcasecmp(argv[0],"syslog-ident") && argc == 2) {
			if (server.syslog_ident) free(server.syslog_ident);
				server.syslog_ident = strdup(argv[1]);
		} else if (!strcasecmp(argv[0],"syslog-facility") && argc == 2) {
			int i;

			for (i = 0; validSyslogFacilities[i].name; i++) {
				if (!strcasecmp(validSyslogFacilities[i].name, argv[1])) {
					server.syslog_facility = validSyslogFacilities[i].value;
					break;
				}
			}

			if (!validSyslogFacilities[i].name) {
				err = "Invalid log facility. Must be one of USER or between LOCAL0-LOCAL7";
				goto loaderr;
			}
		} else if (!strcasecmp(argv[0],"port") && argc == 2) {
			server.port = atoi(argv[1]);
			if (server.port < 0 || server.port > 65535) {
				err = "Invalid port"; goto loaderr;
			}
		} else if (!strcasecmp(argv[0],"bind") && argc >= 2) {
			int j, addresses = argc-1;

			if (addresses > REDD_BINDADDR_MAX) {
				err = "Too many bind addresses specified"; goto loaderr;
			}
			for (j = 0; j < addresses; j++) {
				server.bindaddr[j] = strdup(argv[j+1]);
			}
			server.bindaddr_count = addresses;
		} else if (!strcasecmp(argv[0],"timeout") && argc == 2) {
			server.maxidletime = atoi(argv[1]);
			if (server.maxidletime < 0) {
				err = "Invalid timeout value"; goto loaderr;
			}
		} else if (!strcasecmp(argv[0], "routing-enabled") && argc == 2){
			if ((server.routing_enabled = yesnotoi(argv[1])) == -1) {
				err = "argument must be 'yes' or 'no'"; goto loaderr;
			}
		} else if (!strcasecmp(argv[0],"udp-listen-address") && argc == 2) {
			free(server.udp_listen_address);
			server.udp_listen_address = strdup(argv[1]);
		} else if (!strcasecmp(argv[0], "udp-recv-buf") && argc == 2) {
			server.udp_recv_buf = atoi(argv[1]);
		} else if (!strcasecmp(argv[0], "udp-send-buf") && argc == 2) {
			server.udp_send_buf = atoi(argv[1]);
		} else if (!strcasecmp(argv[0],"save-path") && argc == 2) {
			free(server.save_path);
			server.save_path = strdup(argv[1]);
		} else if (!strcasecmp(argv[0],"vxlan-name") && argc == 2) {
			free(server.vxlan_name);
			server.vxlan_name = strdup(argv[1]);
		} else if (!strcasecmp(argv[0],"vxlan-vni") && argc == 2) {
			free(server.vxlan_vni);
			server.vxlan_vni = strdup(argv[1]);
		} else if (!strcasecmp(argv[0],"vxlan-group") && argc == 2) {
			free(server.vxlan_group);
			server.vxlan_group = strdup(argv[1]);
		} else if (!strcasecmp(argv[0],"vxlan-port") && argc == 2) {
			free(server.vxlan_port);
			server.vxlan_port = strdup(argv[1]);
		} else if (!strcasecmp(argv[0],"vxlan-interface") && argc == 2) {
			free(server.vxlan_interface);
			server.vxlan_interface = strdup(argv[1]);
		} else if (!strcasecmp(argv[0],"vxlan-max-retries") && argc == 2) {
			server.vxlan_max_retries = atoi(argv[1]);
			if (server.vxlan_max_retries < 0) {
				err = "Invalid retry value"; goto loaderr;
			}
		} else {
			err = "Bad directive or wrong number of arguments"; goto loaderr;
		} 
		sdsfreesplitres(argv,argc);
	}
	sdsfreesplitres(lines,totlines);

	return;

loaderr:
	fprintf(stderr, "\n*** FATAL CONFIG FILE ERROR ***\n");
	fprintf(stderr, "Reading the configuration file, at line %d\n", linenum);
	fprintf(stderr, ">>> '%s'\n", lines[i]);
	fprintf(stderr, "%s\n", err);
	exit(1);
}

void
init_server_config(void)
{
	/* General */
	server.configfile			= NULL;
	server.pidfile				= strdup(REDD_DEFAULT_PID_FILE);
	server.daemonize			= REDD_DEFAULT_DAEMONIZE;

	/* Logging */
	server.verbosity			= REDD_DEFAULT_VERBOSITY;
	server.logfile				= strdup(REDD_DEFAULT_LOGFILE);
	server.syslog_enabled		= REDD_DEFAULT_SYSLOG_ENABLED;
	server.syslog_ident			= strdup(REDD_DEFAULT_SYSLOG_IDENT);
	server.syslog_facility		= LOG_LOCAL0;

	/* Networking */
	server.port					= REDD_DEFAULT_SERVERPORT;
	server.ipfd_count			= 0;
	server.maxidletime			= REDD_DEFAULT_MAXIDLETIME;
	server.routing_enabled		= REDD_DEFAULT_ROUTING_ENABLED;
	server.udp_listen_address	= strdup("localhost");
	server.udp_recv_buf			= REDD_DEFAULT_UDP_RECV_BUF;
	server.udp_send_buf			= REDD_DEFAULT_UDP_SEND_BUF;

	/* Database */
	server.nodes				= listCreate();
	listSetDupMethod(server.nodes, dup_node);
	listSetFreeMethod(server.nodes, free_node);
	listSetMatchMethod(server.nodes, match_node);
	server.ips					= listCreate();
	listSetDupMethod(server.ips, dup_ip);
	listSetFreeMethod(server.ips, free_ip);
	listSetMatchMethod(server.ips, match_ip);

	/* VxLan */
	server.vxlan_name			= strdup("vxlan0");
	server.vxlan_vni			= strdup("1");
	server.vxlan_group			= strdup("239.0.0.1");
	server.vxlan_port			= strdup("8472");
	server.vxlan_interface		= strdup("eth0");
	server.vxlan_max_retries	= REDD_DEFAULT_VXLAN_MAX_RETRIES;

	/* Save */
	server.save_path			= strdup(REDD_DEFAULT_SAVE_PATH);
}

/* Load the server configuration from the specified filename.
 * The function appends the additional configuration directives stored
 * in the 'options' string to the config file before loading.
 *
 * Both filename and options can be NULL, in such a case are considered
 * empty. This way load_server_config can be used to just load a file or
 * just load a string. */
void
load_server_config(char *filename, char *options)
{
	sds config = sdsempty();
	char buf[REDD_CONFIGLINE_MAX+1];

	/* Load the file content */
	if (filename) {
		FILE *fp;

		if (filename[0] == '-' && filename[1] == '\0') {
			fp = stdin;
		} else {
			if ((fp = fopen(filename,"r")) == NULL) {
				red_log(REDD_WARNING,
					"Fatal error, can't open config file '%s'", filename);
				exit(1);
			}
		}
		while(fgets(buf,REDD_CONFIGLINE_MAX+1,fp) != NULL)
			config = sdscat(config,buf);
		if (fp != stdin) fclose(fp);
	}
	/* Append the additional options */
	if (options) {
		config = sdscat(config, "\n");
		config = sdscat(config, options);
	}
	load_server_config_from_string(config);
	sdsfree(config);
}

void
clean_server_config()
{
	/* General */
	red_log(REDD_DEBUG, "Cleaning config");
	if (server.configfile)
		sdsfree(server.configfile);
	server.configfile = NULL;
	free(server.pidfile);
	server.pidfile = NULL;

	/* Logging */
	free(server.logfile);
	server.logfile = NULL;
	free(server.syslog_ident);
	server.syslog_ident = NULL;

	/* Networking */
	free(server.udp_listen_address);
	server.udp_listen_address = NULL;

	/* Database */
	listRelease(server.nodes);
	server.nodes = NULL;
	listRelease(server.ips);
	server.ips = NULL;

	/* TUN */
	free(server.tun_name);
	server.tun_name = NULL;

	/* VxLan */
	free(server.vxlan_name);
	server.vxlan_name = NULL;
	free(server.vxlan_vni);
	server.vxlan_vni = NULL;
	free(server.vxlan_group);
	server.vxlan_group = NULL;
	free(server.vxlan_port);
	server.vxlan_port = NULL;
	free(server.vxlan_interface);
	server.vxlan_interface = NULL;

	/* Save */
	free(server.save_path);
	server.save_path = NULL;
}
