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

/* this is the main executable for insight. It provides the websocket handling
 * methods and the methods for collecting the tcp data and converting them
 * into JSON strings for export.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <websock/websock.h> // websocket lib
#include <json/json.h>
#include "estats/debug-int.h"
#include "scripts.h"
#include "main.h"
#include "string-funcs.h"
#include "parse.h"
#include "report.h"

// taken from web10g logger and expanded upon in order to build a JSON data structure
void getConnData (char **message, int skips[], int num, int filter, char** ips, int rcid) {
        struct estats_error* err = NULL;
        struct estats_nl_client* cl = NULL;
        struct estats_connection_list* clist = NULL;
        struct estats_connection* cp;
        struct estats_connection_tuple_ascii asc;
	estats_val_data* tcpdata = NULL;
	char time[20];
	int sport, dport, i, flag = 0;
	

	struct estats_mask mask;

        mask.masks[0] = DEFAULT_PERF_MASK;
        mask.masks[1] = DEFAULT_PATH_MASK;
        mask.masks[2] = DEFAULT_STACK_MASK;
        mask.masks[3] = DEFAULT_APP_MASK;
        mask.masks[4] = DEFAULT_TUNE_MASK;

        for (i = 0; i < MAX_TABLE; i++) {
                mask.if_mask[i] = 0;
        }

	// get a list of the connections available
        Chk(estats_nl_client_init(&cl));
        Chk(estats_connection_list_new(&clist));
	Chk(estats_nl_client_set_mask(cl, &mask));
        Chk(estats_list_conns(clist, cl));
	// create the tcpdata struct
	Chk(estats_val_data_new(&tcpdata));
	
	// This creates the primary json container
	json_object * jsonout = json_object_new_object();
	json_object * data_array = json_object_new_array();
	
	// step through the list of clients
	list_for_each(&clist->connection_head, cp, list) {
		
                struct estats_connection_tuple* ct = (struct estats_connection_tuple*) cp;
		
		// need to use different CHK routine to just go to 
		// Continue rather than Cleanup
                Chk2(estats_connection_tuple_as_strings(&asc, ct));
		Chk2(estats_read_vars(tcpdata, atoi(asc.cid), cl));
		
		// what sort of data filtering will we be using
		switch (filter) {
		case 1: 
			dport = atoi(asc.rem_port);
			sport = atoi(asc.local_port);
                        flag = excludePort(sport, dport, skips, num);
			break;
		case 2:
                        dport = atoi(asc.rem_port);
                        sport = atoi(asc.local_port);
                        // return 0 *if* the dport or sport is in our list of included ports                                                                                            
                        flag = includePort(sport, dport, skips, num);
			break;
		case 3:
			flag = filterIPs(asc.local_addr, asc.rem_addr, ips, num);
			break;
		case 4:
			if (rcid != atoi(asc.cid)) 
				flag = 1;
			break;
		default:
			break;
		}

		if (flag) {
			flag = 0;
			continue;
		}

		// if we have no data then just skip it
		if (tcpdata->length==0)
			continue;

		json_object *connection_data = json_object_new_object(); // main connection array
		json_object *tuple_data = json_object_new_object(); // holds the tuple data

		// create the objects for the tuple data
		json_object *jsrcip = json_object_new_string(asc.local_addr);
		json_object *jsrcport = json_object_new_string(asc.local_port);
		json_object *jdestip = json_object_new_string(asc.rem_addr);
		json_object *jdestport = json_object_new_string(asc.rem_port);
		
		// append the tuple data to the tuple data container
		json_object_object_add (tuple_data, "SrcIP", jsrcip);
		json_object_object_add (tuple_data, "SrcPort", jsrcport);
		json_object_object_add (tuple_data, "DestIP", jdestip);
		json_object_object_add (tuple_data, "DestPort", jdestport);
		
		// append the tupple data container to the connection data container
		json_object_object_add (connection_data, "tuple", tuple_data);

		// add the cid at the head of the data
		json_object *jcid = json_object_new_int(atoi(asc.cid));
		json_object_object_add(connection_data, "cid", jcid);
		// add time data to connection data container
		sprintf (time, "%u.%06u", tcpdata->tv.sec, tcpdata->tv.usec);
		json_object *jtime = json_object_new_string(time);
		json_object_object_add(connection_data, "time", jtime);

		// step through each element of the tcpdata
		// and append it to json list that we appened back on to the head node
		for (i = 0; i < tcpdata->length; i++) {
			if (tcpdata->val[i].masked) 
				continue;
			// the estats->val is a struct which indicated which type of integer value it
			// is but json doesn't care about that so I'm pushing everything over as the 
			// same value type (unsighed 64) this *seems* to work. My guess is that
			// it's all being converted into a string anyway. if that fails at anypoint
			// go back and reinstert the switch statement form any of the userland utilities
			//json_push_back(c, json_new_i(estats_var_array[i].name, tcpdata->val[i].uv64));
			json_object *estats_val = json_object_new_int(tcpdata->val[i].uv64);
			json_object_object_add(connection_data,estats_var_array[i].name, estats_val);
		}
		// add the connection data to the primary container tagged with the connection id
		json_object_array_add(data_array, connection_data);
	Continue:       
		while(0) {}
	}
	json_object_object_add(jsonout, "DATA", data_array); 
		
       	// convert the json object to a string
	*message = malloc(strlen(json_object_to_json_string(jsonout)+1) * sizeof(*message));
	strcpy(*message, (char *)json_object_to_json_string(jsonout));
	//*message =  (char *)json_object_to_json_string(jsonout);
	
	// free the json object from the root node
	json_object_put(jsonout);

Cleanup:
        estats_connection_list_free(&clist);
	estats_val_data_free(&tcpdata);
        estats_nl_client_destroy(&cl);
	
        if (err != NULL) {
		PRINT_AND_FREE(err);
                printf ("EXIT_FAILURE");
        }
        
        return;
}

// figure out what we are doing with the incoming requests
void *analyzeInbound(libwebsock_client_state *state, libwebsock_message *msg)  
{
	// store the data as a char
	char *request = msg->payload;
        char *message;
	int ports[20] = {};
	char **ips = NULL;
	int num = 0; 
	int i = 0;
	int cid = 0;
	int filter = 0;
	int free_flag = 0;
	json_tokener *tok = json_tokener_new();
	json_object *json_in = NULL;
	json_object *cmnd_obj = NULL;
	json_object *optn_obj = NULL;
	enum json_tokener_error jerr;
	reportinfo *report_t;

	// grab the incoming json string
	// it should be of the form 
	// {"index": { "command": <command>, "options": {"opt1":<option>,...}}}
	// or {array{string, array{strings}}}
	json_in = json_tokener_parse_ex(tok, request, strlen(request));
	while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);
	if (jerr != json_tokener_success)
	{
		// it doesn't seem to be a json object
		// at this point we'll just return null and wait for the next message
		fprintf(stderr, "Inbound JSON Error: %s\n", json_tokener_error_desc(jerr));
		return NULL;
	}
	
	if (tok->char_offset < strlen(request)) // XXX shouldn't access internal fields
	{
		// this is when we get characters appended to the end of the json object
		// since this shouldn't be sent from the UI we'll just return null and wait for 
		// a new message
		fprintf(stderr, "Poorly formed JSON object. Check for extraneous characters.");
		return NULL;
	}	 
	// we now have a validated inbound json object. 
	// use that object and pass a reference to it to the command 
	json_object_object_get_ex(json_in, "command", &cmnd_obj);

	// convert into a string so we can use it for the strpos later on
	char * command = (char *)json_object_get_string(cmnd_obj);
	printf("command is '%s'\n", command);
		      
	// list all data
	if (strcmp(command, "list") == 0) {
	}
	// exlude all connections matching the listed ports
	else if (strcmp(command, "exclude") == 0) {
		json_object_object_get_ex(json_in, "options", &optn_obj);
		char * excluded = (char *)json_object_get_string(optn_obj);
		printf("options are %s\n", excluded);
		// set to 1 to exclude the listed ports from the report
		filter = 1;
		// get the ports and number of items in array
		num = parsePorts(ports, excluded);
	}
	// include *only* the connections matching the listed ports
	else if (strcmp(command, "include") == 0) {
		json_object_object_get_ex(json_in, "options", &optn_obj);
		char * included = (char *)json_object_get_string(optn_obj);
		printf("options are %s\n", included);
		// set to 2 to only report on the listed ports
		filter = 2;
		num = parsePorts(ports, included);
	}
	else if (strcmp(command, "ipfilter") == 0) {
		// allocate space for the IPs
		free_flag = 1;
		ips = malloc(20 * sizeof *ips);
		for (i = 0; i < 20; i++) {
			ips[i] = malloc (20 * sizeof *ips[i]);
		}
		// get the json object
		json_object_object_get_ex(json_in, "options", &optn_obj);
		char * filtered = (char *)json_object_get_string(optn_obj);
		printf("options are %s\n", filtered);
		filter = 3;
		num = parseIPs(ips, filtered);
	} else if (strcmp(command, "report") == 0) {
		report_t = (reportinfo *)malloc(sizeof(reportinfo)); 
		json_object_object_get_ex(json_in, "options", &optn_obj);
		// now we need to get the json object that is the noc/db access information
		// parse it out, get the latest stats for the cid, package them, and send them over
		// pass the object to json_parse_report
		report_parse(optn_obj, report_t);
		// dump the values to the log screen - remove for production
		report_dump(report_t);
		filter = 4;
		getConnData(&message, ports, num, filter, ips, report_t->cid);
		// getConnData doesn't return a json object so we'll
		// need to convert the message back into a json object
		// at some point
		report_sql(report_t, message);
		libwebsock_send_text_with_length(state, message, strlen(message));
		report_free(report_t);
		return NULL;
	} else {
		return NULL;
	}
	getConnData(&message, ports, num, filter, ips, cid);
	printf("%s",message);
	libwebsock_send_text_with_length(state, message, strlen(message));
	
	// free the inbound jason object and token
	json_object_put(json_in);
	json_tokener_free(tok);
	free(message);

	// if we used ip filter we need to free the memory. 
	if (free_flag) {
		for (i = 0; i < 20; i++) {
			free(ips[i]);
		}
		free(ips);
		free_flag = 0;
	}
	return NULL;
}


int
onmessage(libwebsock_client_state *state, libwebsock_message *msg)
{
	switch (msg->opcode) {
	case WS_OPCODE_TEXT:{
		analyzeInbound(state, msg);
		break;
	}
	default:
		fprintf(stderr, "Unknown opcode: %d\n", msg->opcode);
		break;
	}
	return 0;
}

int
onopen(libwebsock_client_state *state)
{
	fprintf(stderr, "onopen: %d\n", state->sockfd);
	return 0;
}

int
onclose(libwebsock_client_state *state)
{
	fprintf(stderr, "onclose: %d\n", state->sockfd);
	return 0;
}

int main(int argc, char *argv[])
{
	libwebsock_context *ctx = NULL;
	char* port;

	if (argc != 2) {
		port = "9000";
	} else {
		port = argv[1];
	}
	
	ctx = libwebsock_init();
	if (ctx == NULL ) {
		fprintf(stderr, "Error during libwebsock_init.\n");
		exit(1);
	}
	libwebsock_bind(ctx, "0.0.0.0", port);
	fprintf(stderr, "libwebsock listening on port %s\n", port);
	ctx->onmessage = onmessage;
	ctx->onopen = onopen;
	ctx->onclose = onclose;
	libwebsock_wait(ctx);
	//perform any cleanup here.
	fprintf(stderr, "Exiting.\n");
	return 0;
}