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

//void getConnData (char **, int [], int, int, char**, int, char *);
//void getConnData (char **, struct FilterList *);
void *analyzeInbound(libwebsock_client_state *, libwebsock_message *);
int onmessage(libwebsock_client_state *, libwebsock_message *);
int onopen(libwebsock_client_state *);
int onclose(libwebsock_client_state *);
