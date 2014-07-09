/*
 * Copyright (c) 2013 The Board of Trustees of Carnegie Mellon University.
 *
 *  Author: Chris Rapier <rapier@psc.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT License.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the MIT License for more details.
 *
 * You should have received a copy of the MIT License along with this library;
 * if not, see http://opensource.org/licenses/MIT.
 *
 */

#include <websock/websock.h> // websocket lib
#include "uthash.h"
#include "parse.h"

void get_metric_mask (struct estats_mask *, char *);
void add_cmdline(int, char *);
void get_cmdline_from_cid(char ** , int);
void get_connection_data (char **, struct FilterList *);
void *analyze_inbound(libwebsock_client_state *, libwebsock_message *);
int onmessage(libwebsock_client_state *, libwebsock_message *);
int onopen(libwebsock_client_state *);
int onclose(libwebsock_client_state *);

typedef struct CmdLineCID {
	int cid;
	char cmdline[256];
	UT_hash_handle hh;
} CmdLineCID;
