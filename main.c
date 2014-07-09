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
#include <json-c/json.h>
#include "estats/debug-int.h"
#include "scripts.h"
#include "main.h"
#include "string-funcs.h"
#include "parse.h"
#include "report.h"
#include "geoip.h"
#include "version.h"
#include "debug.h"
#include "uthash.h"

int debugflag = 0;
int printjson = 0;

MMDB_s mmdb; //define the db handle as a global. possibly bad form but we can revist

struct CmdLineCID *cmdlines = NULL;

void getMetricMask (struct estats_mask *mask, char *maskstring) {
	const char *defaultmask = "25124,12,410000,90,0"; //if not mask passed by client UI use this
        char *strmask = NULL; // mask string
	char *cp_strmask = NULL; // copy of mask string
        uint64_t tmpmask = NULL;
        const char delim = ',';
	int i = 0;

	if (maskstring != NULL) {
		int masklength = strlen(maskstring);
		// if the length of the mask is 0 then treat that as no passed mask
		// otherwise it will default to a full report
		if (masklength != 0) {
			strmask = strdup(maskstring);
		} else {
			strmask = strdup(defaultmask);
		}
	} else {
		strmask = strdup(defaultmask);
	}
	
	// we need to maintain a copy of the pointer of strmask so we can free it
	cp_strmask = strmask;
	
	// initialize the mask struct
        mask->masks[0] = DEFAULT_PERF_MASK;
        mask->masks[1] = DEFAULT_PATH_MASK;
        mask->masks[2] = DEFAULT_STACK_MASK;
        mask->masks[3] = DEFAULT_APP_MASK;
        mask->masks[4] = DEFAULT_TUNE_MASK;

        for (i = 0; i < MAX_TABLE; i++) {
                mask->if_mask[i] = 0;
        }

	// convert the mask string into the mask struct
	for (i = 0; i < 5; i++) {
		char *strtmp;
		strtmp = strsep(&strmask, &delim);
		if (strtmp && strlen(strtmp)) {
			char *str;
			str = (str = strchr(strtmp, 'x')) ? str+1 : strtmp;
			if (sscanf(str, "%"PRIx64, &tmpmask) == 1) {
				mask->masks[i] = tmpmask & mask->masks[i];
				mask->if_mask[i] = 1;
			}
		}
	}
	free(cp_strmask); // actually frees strmask
}

void add_cmdline(int cid, char *cmdline) {
    struct CmdLineCID *s;

    HASH_FIND_INT(cmdlines, &cid, s);
    if (s == NULL) {
	    s = (struct CmdLineCID*)malloc(sizeof(struct CmdLineCID));
	    s->cid = cid;
	    HASH_ADD_INT(cmdlines, cid, s);
    }
    strcpy(s->cmdline, cmdline);
    return;
}

void get_cmdline_from_cid(char** appname, int cid) {
	struct CmdLineCID *s;
	HASH_FIND_INT(cmdlines, &cid, s);
	if (s != NULL) {
		*appname = strdup(s->cmdline);
	} else {
		*appname = strdup("\0");
	}
	return;
}

// taken from web10g logger and expanded upon in order to build a JSON data structure
void getConnData (char **message, struct FilterList *filterlist) {
        struct estats_error* err = NULL;
        struct estats_nl_client* cl = NULL;
        struct estats_connection_list* clist = NULL;
        struct estats_connection* cp = NULL;
        struct estats_connection_info* ci;
	struct estats_connection_tuple_ascii asc;
	estats_val_data* tcpdata = NULL;
	char time[20];
	int sport, dport, i, flag, shownconn = 0;
        struct estats_mask mask; // mask struct
	char *maskstring = filterlist->mask;
	enum RequestTypes request;
	char *appname = NULL;
	int maxconn = 0;

	// compute the mask based on the maskstring
	getMetricMask(&mask, maskstring);

	// get a list of the connections available
        Chk(estats_nl_client_init(&cl));
        Chk(estats_connection_list_new(&clist));
	Chk(estats_nl_client_set_mask(cl, &mask));
        Chk(estats_list_conns(clist, cl));
	Chk(estats_connection_list_add_info(clist));

	// create the tcpdata struct
	Chk(estats_val_data_new(&tcpdata));
	
	// This creates the primary json container
	json_object * jsonout = json_object_new_object();
	json_object * data_array = json_object_new_array();

	// build a hash of the cmdlines and cids
	list_for_each(&clist->connection_info_head, ci, list) {
		add_cmdline(ci->cid, ci->cmdline);
	}
	
	// step through the list of clients
	list_for_each(&clist->connection_head, cp, list) {
		maxconn++; // total number of connections processed including skipped connections
		
                struct estats_connection_tuple* ct = (struct estats_connection_tuple*) cp;

		get_cmdline_from_cid(&appname, atoi(asc.cid));

		// need to use different CHK routine to just go to 
		// Continue rather than Cleanup
                Chk2Ign(estats_connection_tuple_as_strings(&asc, ct));
		Chk2Ign(estats_read_vars(tcpdata, atoi(asc.cid), cl));

		// we have to go through this for each connection
		for (i = 0; i < filterlist->maxindex; i++) {
			request = parse_string_to_enum(filterlist->commands[i]);
			// what sort of data filtering will we be using
			switch (request) {
			case exclude: 
				dport = atoi(asc.rem_port);
				sport = atoi(asc.local_port);
				flag = excludePort(sport, dport, filterlist->ports[i], 
						   filterlist->arrindex[i]);
				break;
			case include:
				dport = atoi(asc.rem_port);
				sport = atoi(asc.local_port);
				// return 0 *if* the dport or sport is in our list of included ports
				flag = includePort(sport, dport, filterlist->ports[i], 
						   filterlist->arrindex[i]);
				break;
			case filterip:
				flag = filterIPs(asc.local_addr, asc.rem_addr, 
						 filterlist->strings[i], filterlist->arrindex[i]);
				break;
			case report:
				if (filterlist->reportcid != atoi(asc.cid)) 
					flag = 1;
				break;
			case appinclude:
				flag = includeApp(appname, filterlist->strings[i], 
						  filterlist->arrindex[i]);
				break;
			case appexclude:
				flag = excludeApp(appname, filterlist->strings[i], 
						  filterlist->arrindex[i]);
				break;
			case list:
				break;
			default:
				break;
			}
			
		}
		
		// if any of the commands creates a flag we skip it. 
		if (flag) {
			free(appname);
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
		json_object *jappname = json_object_new_string(appname);
		
		// append the tuple data to the tuple data container
		json_object_object_add (tuple_data, "SrcIP", jsrcip);
		json_object_object_add (tuple_data, "SrcPort", jsrcport);
		json_object_object_add (tuple_data, "DestIP", jdestip);
		json_object_object_add (tuple_data, "DestPort", jdestport);
		json_object_object_add (tuple_data, "Application", jappname);
		free(appname);

		// append the tupple data container to the connection data container
		json_object_object_add (connection_data, "tuple", tuple_data);

		// add the cid at the head of the data
		json_object *jcid = json_object_new_int(atoi(asc.cid));
		json_object_object_add(connection_data, "cid", jcid);
		// add time data to connection data container
		sprintf (time, "%u.%06u", tcpdata->tv.sec, tcpdata->tv.usec);
		json_object *jtime = json_object_new_string(time);
		json_object_object_add(connection_data, "time", jtime);

		// get the latitude and longitude of the remote address
		double latitude = geoip_lat_or_long(asc.rem_addr, mmdb, "latitude");
		double longitude = geoip_lat_or_long(asc.rem_addr, mmdb, "longitude");

		// printf("%s, lat: %f, long: %f\n", asc.rem_addr, latitude, longitude);
		json_object *jlat = json_object_new_double(latitude);
		json_object *jlong = json_object_new_double(longitude);
	
		json_object_object_add(connection_data, "lat", jlat);
		json_object_object_add(connection_data, "long", jlong);

		// step through each element of the tcpdata
		// and append it to json list that we appened back on to the head node
		for (i = 0; i < tcpdata->length; i++) {
			if (tcpdata->val[i].masked) 
				continue;

			// this switch is likely unnecessary as all estat ints are cast to 
			// int64 however, just to be on the safe side I've implemented this
			json_object *estats_val = NULL;
			switch(estats_var_array[i].valtype) {
			case ESTATS_UNSIGNED64:
				estats_val = json_object_new_int64(tcpdata->val[i].uv64);
				break;
			case ESTATS_UNSIGNED32:
				estats_val = json_object_new_int64(tcpdata->val[i].uv32);
				break;
			case ESTATS_SIGNED32:
				estats_val = json_object_new_int64(tcpdata->val[i].sv32);
				break;
			case ESTATS_UNSIGNED16:
				estats_val = json_object_new_int64(tcpdata->val[i].uv16);
				break;
			case ESTATS_UNSIGNED8:
				estats_val = json_object_new_int64(tcpdata->val[i].uv8);
				break;
			default:
				break;
			} //end switch
			if (estats_val != NULL) 
				json_object_object_add(connection_data,estats_var_array[i].name, estats_val);
		} //end for loop
		// add the connection data to the primary container tagged with the connection id
		json_object_array_add(data_array, connection_data);
		shownconn++; // how many connections we actually processed the tcpdata of
Continue:       
		while(0) {}
	}
	json_object_object_add(jsonout, "DATA", data_array); 
		
       	// convert the json object to a string
	*message = malloc(strlen(json_object_to_json_string(jsonout)+1) * sizeof(*message));
	strcpy(*message, (char *)json_object_to_json_string(jsonout));
	
	log_info("%s\n", *message);
	log_debug("Processed %d of %d connections", shownconn, maxconn);
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
	char **ips = NULL;
	int i, j = 0;
	int free_flag = 0;
	json_tokener *tok = json_tokener_new();
	json_object *json_in = NULL;
	enum json_tokener_error jerr;
	enum RequestTypes requests;
	CommandList *comlist;
	FilterList *filterlist; // this contains the parsed commands

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
	// fill the comlist struct with our incoming request
	comlist = malloc(sizeof(CommandList));
	comlist->mask = NULL;
	comlist->maxindex = 0;
	json_parse(json_in, comlist);
	
	if (debugflag) {
		for (i = 0; i < comlist->maxindex; i++) {
			log_debug("[%d]command: %s", i, comlist->commands[i]);
			log_debug("[%d]options: %s", i, comlist->options[i]);
		}
		log_debug("mask: %s", comlist->mask);
	}

	// filterlist is what we will be using to, ya know, filter the data. 
	filterlist = malloc(sizeof(FilterList));
	parse_comlist(comlist, filterlist);

	// print out what we have for debug purposes
	if (debugflag) {
		log_debug ("INBOUND COMMANDS");
		if (filterlist->mask != NULL)
			log_debug("mask: %s", filterlist->mask);
		log_debug("MaxIndex: %d", filterlist->maxindex);
		for (i = 0; i < filterlist->maxindex; i++) {
			log_debug("[%d]command in filterlist is %s", i, filterlist->commands[i]);
			log_debug("[%d]array length is %d", i, filterlist->arrindex[i]);
			log_debug("[%d]array elements: ", i);
			for (j = 0; j < filterlist->arrindex[i]; j++) {
				if (strcmp(filterlist->commands[i], "exclude") == 0) {
					if (filterlist->ports[i][j] != -1){
						log_debug("\t[%d][%d] %d", i, j, filterlist->ports[i][j]);
					}
				}
				if (strcmp(filterlist->commands[i], "include") == 0) {
					if (filterlist->ports[i][j] != -1){
						log_debug("\t[%d][%d] %d", i, j, filterlist->ports[i][j]);
					}
				}
				if(strcmp(filterlist->commands[i], "filterip") == 0) {
					if (filterlist->strings[i][j] != NULL)
						log_debug("\t[%d][%d] %s", i, j, filterlist->strings[i][j]);
				}
				if(strcmp(filterlist->commands[i], "appexclude") == 0) {
					if (filterlist->strings[i][j] != NULL)
						log_debug("\t[%d][%d] %s", i, j, filterlist->strings[i][j]);
				}

				if(strcmp(filterlist->commands[i], "appinclude") == 0) {
					if (filterlist->strings[i][j] != NULL)
						log_debug("\t[%d][%d] %s", i, j, filterlist->strings[i][j]);
				}
			}
		}
	}
	// we now have a struct that contains all of the 
	// various commands, arrays, and index values

	getConnData(&message, filterlist); 
	log_debug("message length: %d",(int)strlen(message)); 
	libwebsock_send_text_with_length(state, message, strlen(message));
	
	for (i = 0; i < comlist->maxindex; i++) {
	  free(comlist->commands[i]);
	  free(comlist->options[i]);
	}
	free(comlist->mask);
	free(comlist);
	


	// we have to go through this for each connection
	for (i = 0; i < filterlist->maxindex; i++) {
		requests = parse_string_to_enum(filterlist->commands[i]);
		// what sort of data filtering will we be using
		switch (requests) {
		case exclude: 
		case include:
			free(filterlist->commands[i]);
			free(filterlist->ports[i]);
			break;
		case filterip:
		case appexclude:
		case appinclude:
			for (j = 0; j < filterlist->arrindex[i]; j++)
				free(filterlist->strings[i][j]);
			free(filterlist->strings[i]);
			free(filterlist->commands[i]);
			break;
		case report:
			break;
		case list:
			break;
		default:
			break;
		}
		
	}
	free(filterlist->mask);
	free(filterlist);

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

void usage(void) {
	printf ("Insight Web10G client. Version: %3.2f\n", VERSION);
	printf ("\t-h this help screen\n");
	printf ("\t-p listen port\n");
	printf ("\t-g path to geoip database\n");
	printf ("\t-d print debug statements to stdout\n");
	printf ("\t-j print outbound json string to stdout\n");
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
	char* port = "9000";
	char* geoippath = "/home/rapier/websockets/test/GeoLite2-City.mmdb";
	int opt;

        while ((opt = getopt(argc, argv, "hp:g:dj")) != -1) {
                switch (opt) {
                case 'h':
                        usage();
                        exit(EXIT_SUCCESS);
                        break;
                case 'p':
			port = optarg;
                        break;
                case 'g':
                        geoippath = optarg;
                        break;
		case 'd':
			debugflag = 1;
			break;
		case 'j':
			printjson = 1;
			break;
                default:
			usage();
                        exit(EXIT_FAILURE);
                        break;
                }
        }

	int status = MMDB_open(geoippath, MMDB_MODE_MMAP, &mmdb);
	if (MMDB_SUCCESS != status) {
	  printf("Can't open db\n");
	  return 0;
	}

	printf ("geoIP database opened and ready\n");

	ctx = libwebsock_init();
	if (ctx == NULL ) {
		fprintf(stderr, "Error during libwebsock_init.\n");
		exit(1);
	}
	libwebsock_bind(ctx, "127.0.0.1", port);
	fprintf(stderr, "libwebsock listening on port %s\n", port);
	ctx->onmessage = onmessage;
	ctx->onopen = onopen;
	ctx->onclose = onclose;
	libwebsock_wait(ctx);
	//perform any cleanup here.
	fprintf(stderr, "Exiting.\n");
	MMDB_close(&mmdb);
	return 0;
}
