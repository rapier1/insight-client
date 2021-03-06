
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
#include "scripts.h"
#include "main.h"

int debugflag = 0;
int printjson = 0;

#define INTERVAL 180

MMDB_s geoipdb; //define the db handle as a global. possibly bad form but we can revist

/* we have to define the report_list linked list as global because the callback
 * for websocket doesn't give us anyway to pass extra variables. That's our only entry
 * into analyzing the inbound message. Not happy about this.
 */
reports_ll *report_list_head = NULL;


void get_metric_mask (struct estats_mask *mask, char *maskstring) {
	const char *defaultmask = ",,,,,"; //if not mask passed by UI use this
	char *strmask = NULL; // mask string
	char *cp_strmask = NULL; // copy of mask string
	uint64_t tmpmask;
	const char delim = ',';
	int i = 0;
	int masklength = 0;

	if (maskstring != NULL) {
		masklength = strlen(maskstring);
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
	// as the ponter is shifted by strsep
	cp_strmask = strmask;

	// initialize the mask struct
	mask->masks[0] = DEFAULT_PERF_MASK;
	mask->masks[1] = DEFAULT_PATH_MASK;
	mask->masks[2] = DEFAULT_STACK_MASK;
	mask->masks[3] = DEFAULT_APP_MASK;
	mask->masks[4] = DEFAULT_TUNE_MASK;
	mask->masks[5] = DEFAULT_EXTRAS_MASK;

	// convert the mask string into the mask struct
	for (i = 0; i < MAX_TABLE; i++) {
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

void add_cmdline_to_hash(int cid, char *cmdline, CmdLineCID **cmdlines) {
	struct CmdLineCID *s;

	HASH_FIND_INT(*cmdlines, &cid, s);
	if (s == NULL) {
		s = (struct CmdLineCID*)malloc(sizeof(struct CmdLineCID));
		s->cid = cid;
		HASH_ADD_INT(*cmdlines, cid, s);
	}
	strcpy(s->cmdline, cmdline);
	return;
}

void get_cmdline_from_cid_hash(char** appname, int cid, CmdLineCID **cmdlines) {
	struct CmdLineCID *s;
	HASH_FIND_INT(*cmdlines, &cid, s);
	if (s != NULL) {
		*appname = strdup(s->cmdline);
	} else {
		*appname = strdup("\0");
	}
	return;
}

void delete_all_from_hash(CmdLineCID **cmdlines) {
	struct CmdLineCID *current_cmd, *tmp;

	HASH_ITER(hh, *cmdlines, current_cmd, tmp) {
		HASH_DEL(*cmdlines, current_cmd);  /* delete it (cmdlines advances to next) */
		free(current_cmd);            /* free it */
	}
}

// taken from web10g logger and expanded upon in order to build a JSON data structure
void get_connection_data (char **message, struct FilterList *filterlist) {
	struct estats_error* err = NULL;
	struct estats_nl_client* cl = NULL;
	struct estats_connection_list* clist = NULL;
	struct estats_connection* cp = NULL;
	struct estats_connection_info* ci;
	struct estats_connection_tuple_ascii asc;
	struct CmdLineCID *cmdlines = NULL;
	estats_val_data* tcpdata = NULL;
	char time[20];
	int sport, dport, i, flag, shownconn = 0;
	struct estats_mask mask; // mask struct
	char *maskstring = filterlist->mask;
	enum RequestTypes request;
	char *appname = NULL;
	int maxconn = 0;

	// compute the mask based on the maskstring
	get_metric_mask(&mask, maskstring);

	// get a list of the connections available
	Chk(estats_nl_client_init(&cl)); //init the netlink client
	Chk(estats_connection_list_new(&clist)); //create a new connection list
	Chk(estats_nl_client_set_mask(cl, &mask)); //set the metric mask
	Chk(estats_list_conns(clist, cl)); // fill the connection list with cid data
	Chk(estats_connection_list_add_info(clist)); //add the application, pid, name data

	// create the tcpdata struct
	Chk(estats_val_data_new(&tcpdata));

	// This creates the primary json container
	json_object * jsonout = json_object_new_object();
	json_object * data_array = json_object_new_array();

	// build a hash of the cmdlines and cids
	list_for_each(&clist->connection_info_head, ci, list) {
		add_cmdline_to_hash(ci->cid, ci->cmdline, &cmdlines);
	}

	// step through the list of clients
	list_for_each(&clist->connection_head, cp, list) {
		maxconn++; // total number of connections processed including skipped connections
		struct estats_connection_tuple* ct = (struct estats_connection_tuple*) &(cp->rem_addr[0]);

		// need to use different CHK routine to just go to 
		// Continue rather than Cleanup
		Chk2Ign(estats_connection_tuple_as_strings(&asc, ct));
		Chk2Ign(estats_read_vars(tcpdata, atoi(asc.cid), cl));

		// if we have no data then just skip it
		if (tcpdata->length==0)
			continue;

		// We really don't care about connections to localhost
		// TODO this should be turned into the entire 127/8 block
		if (strcmp(asc.rem_addr, "127.0.0.1") == 0)
			continue;

		get_cmdline_from_cid_hash(&appname, atoi(asc.cid), &cmdlines);

		// we don't want to report on our own connection to the websocket. that's silly.
		if (strcmp(appname, "insight") == 0) {
			free(appname);
			continue;
		}

		// we have to go through this for each connection
		// as a note. This really end up being just a great big OR statement
		// so conflicting commands will result in no data or all the data
		// it's up to the user and/or ui to make smart choices.
		for (i = 0; i < filterlist->maxindex; i++) {
			request = parse_string_to_enum(filterlist->commands[i]);
			// what sort of data filtering will we be using
			switch (request) {
			case list:
				break;
			case exclude:
				dport = atoi(asc.rem_port);
				sport = atoi(asc.local_port);
				flag = exclude_port(sport, dport, filterlist->ports[i],
						filterlist->arrindex[i]);
				break;
			case filterip:
				flag = filter_ips(asc.local_addr, asc.rem_addr,
						filterlist->strings[i], filterlist->arrindex[i]);
				break;
			case appinclude:
				flag = include_app(appname, filterlist->strings[i],
						filterlist->arrindex[i]);
				break;
			case include:
				dport = atoi(asc.rem_port);
				sport = atoi(asc.local_port);
				// return 0 *if* the dport or sport is in our list of included ports
				flag = include_port(sport, dport, filterlist->ports[i],
						filterlist->arrindex[i]);
				break;
			case appexclude:
				flag = exclude_app(appname, filterlist->strings[i],
						filterlist->arrindex[i]);
				break;
			case report:
				if (filterlist->reportcid != atoi(asc.cid))
					flag = 1;
				break;
			default:
				break;
			}
			if (flag) {
				free(appname);
				flag = 0;
				goto Continue;
			}
		}

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
		free(appname); // no longer need the name of the application

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
		double latitude = geoip_lat_or_long(asc.rem_addr, geoipdb, "latitude");
		double longitude = geoip_lat_or_long(asc.rem_addr, geoipdb, "longitude");

		double local_lat = geoip_lat_or_long(asc.local_addr, geoipdb, "latitude");
		double local_long = geoip_lat_or_long(asc.local_addr, geoipdb, "longitude");

		json_object *jlat = json_object_new_double(latitude);
		json_object *jlong = json_object_new_double(longitude);

		json_object *jloc_lat = json_object_new_double(local_lat);
		json_object *jloc_long = json_object_new_double(local_long);

		json_object_object_add(connection_data, "rem_lat", jlat);
		json_object_object_add(connection_data, "rem_long", jlong);

		json_object_object_add(connection_data, "loc_lat", jloc_lat);
		json_object_object_add(connection_data, "loc_long", jloc_long);


		// step through each element of the tcpdata
		// and append it to json list that we appened back on to the head node
		for (i = 0; i < tcpdata->length; i++) {
			if (tcpdata->val[i].masked) 
				continue;

			// there is an UNSIGNED16 defined but no values make use of it
			// case statements in order of frequency
			json_object *estats_val = NULL;
			switch(estats_var_array[i].valtype) {
			case ESTATS_UNSIGNED32:
				estats_val = json_object_new_int64(tcpdata->val[i].uv32);
				break;
			case ESTATS_SIGNED32:
				estats_val = json_object_new_int64(tcpdata->val[i].sv32);
				break;
			case ESTATS_UNSIGNED64:
				estats_val = json_object_new_int64(tcpdata->val[i].uv64);
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

	// if they want the json string sent to stdout then make it pretty otherwise 
	// skip the extra characters. The UI should handle it either way without a problem
	if (printjson)
		*message = strdup((char *)json_object_to_json_string_ext(jsonout, JSON_C_TO_STRING_PRETTY));
	else
		*message = strdup((char *)json_object_to_json_string_ext(jsonout, JSON_C_TO_STRING_PLAIN));

	log_info("%s\n", *message);
	log_debug("Processed %d of %d connections", shownconn, maxconn);
	// free the json object from the root node
	json_object_put(jsonout);

Cleanup:
	estats_connection_list_free(&clist);
	estats_val_data_free(&tcpdata);
	estats_nl_client_destroy(&cl);
	delete_all_from_hash(&cmdlines); // free the command line hash
	if (err != NULL) {
		PRINT_AND_FREE(err);
	}
	return;
}

/* Before we go through the process of getting the data we
 * need to make sure the CID exists in the current list of connections
 */
int confirm_cid (int cid) {
	struct estats_error* err = NULL;
	struct estats_nl_client* cl = NULL;
	struct estats_connection_list* clist = NULL;
	struct estats_connection* cp = NULL;
	struct estats_connection_tuple_ascii asc;
	int myerr = 0;

	// get a list of the connections available
	Chk(estats_nl_client_init(&cl)); //init the netlink client
	Chk(estats_connection_list_new(&clist)); //create a new connection list
	Chk(estats_list_conns(clist, cl)); // fill the connection list with cid data

	// step through the list of connections
	list_for_each(&clist->connection_head, cp, list) {
		struct estats_connection_tuple* ct = (struct estats_connection_tuple*) cp;

		// need to use different CHK routine to just go to 
		// Continue rather than Cleanup
		Chk2Ign(estats_connection_tuple_as_strings(&asc, ct));
		if (atoi(asc.cid) == cid) {
			myerr = 1;
			goto Cleanup;
		}
	Continue:
		while (0) {}
	}
Cleanup:
	estats_connection_list_free(&clist);
	estats_nl_client_destroy(&cl);
	return myerr;
}

// figure out what we are doing with the incoming requests
// we take the incoming string and tokenize it into a json object
// we then convert the json object into a struct holding all of the
// commands. this struct is then converted into a filterlist which
// is better suited for the filter routines we have
void *analyze_inbound(libwebsock_client_state *state, libwebsock_message *msg)  
{
	// store the data as a char
	char *request = msg->payload;
	char *message;
	int i, j = 0;
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
	log_debug("REQUEST: %s\n", request);
	json_in = json_tokener_parse_ex(tok, request, strlen(request));
	while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);
	if (jerr != json_tokener_success)
	{
		// it doesn't seem to be a json object
		// at this point we'll just return null and wait for the next message
		fprintf(stderr, "Inbound JSON Error: %s\n", json_tokener_error_desc(jerr));
		return NULL;
	}

	if (tok->char_offset < strlen(request)) // shouldn't access internal fields
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
	parse_json(json_in, comlist);

	if (comlist->maxindex == 0 ) {
		// they sent an empty command list. most likely they 
		// just sent a 'mask' line so we'll assume they want a 
		// 'list'
		comlist->maxindex = 1;
		comlist->commands[0] = strdup("list");
		comlist->options[0] = strdup("n/a");
	}

	if (debugflag) {
		for (i = 0; i < comlist->maxindex; i++) {
			log_debug("[%d]command: %s", i, comlist->commands[i]);
			log_debug("[%d]options: %s", i, comlist->options[i]);
		}
		log_debug("mask: %s", comlist->mask);
	}

	if (strcmp(comlist->commands[0], "report") == 0) {
		char *response;
		// TODO the reporting system needs to be changed in order to 
		// reflect the new options
		if(report_create_from_json_string(comlist->options[0], &report_list_head) != 1){
			//create a special json string to indicate that there
			//was an error with the attempt to make a report
			response = "{\"function\":\"report\", \"result\":\"failure\"}";
		} else {
			response = "{\"function\":\"report\", \"result\":\"success\"}";
		}
		// send the result back to the client and then cleanup
		if (libwebsock_send_text_with_length(state, response, strlen(response)) != 0)
			printf("Failure to send report response to user interface\n");
		// done with comlist so free it up now. 
		for (i = 0; i < comlist->maxindex; i++) {
			free(comlist->commands[i]);
			free(comlist->options[i]);
		}
		free(comlist->mask);
		free(comlist);
		goto Cleanup;
	}

	// if the linked list exists step through it
	if (report_list_head != NULL) {
		struct reports_ll *curr = report_list_head;
		struct timeval time_s;
		time_t cur_sec;
		int update_interval;
		gettimeofday(&time_s, NULL);
		cur_sec = time_s.tv_sec;
		while (curr != NULL) {
			// only send a report to the DB if we've exceeded the defined reporting interval
			if (curr->report->interval <= 0) 
				update_interval = INTERVAL;
			else 
				update_interval = curr->report->interval;
			if (cur_sec - curr->report->update_time > update_interval) {
				printf ("Examining CID: %d\n", curr->report->cid);
				if (confirm_cid(curr->report->cid)) {
					if (report_execute(curr->report) == 1) {
						curr->report->update_time = cur_sec;
						printf("executed\n");
						/* TODO send the UI a notice that the report was sent */
					}
				} else {
					/* the cid no long exists so we should remove it from the linked list */
					if (report_del_cid(curr->report->cid, &report_list_head) == 1) {
						printf ("CID not found, removed from linked list\n");
						/* TODO send the UI a notice that this has happened */
					}
				}
			}
			curr = curr->next;
		}
	}

	// filterlist is what we will be using to, ya know, filter the data. 
	filterlist = malloc(sizeof(FilterList));
	parse_comlist(comlist, filterlist);

	// done with comlist so free it up now. 
	for (i = 0; i < comlist->maxindex; i++) {
		free(comlist->commands[i]);
		free(comlist->options[i]);
	}
	free(comlist->mask);
	free(comlist);

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
	// so build the json string
	get_connection_data(&message, filterlist); 

	log_debug("message length: %d",(int)strlen(message)); 

	// now we send it to the socket 
	if (libwebsock_send_text_with_length(state, message, strlen(message)) != 0)
		printf ("Failure in sending data to user interface.\n");

	// free up the filterlist struct we use a case statement here
	// because not everything is malloc'd and we don't want a bad free
	for (i = 0; i < filterlist->maxindex; i++) {
		requests = parse_string_to_enum(filterlist->commands[i]);
		free(filterlist->commands[i]);
		// what sort of data filtering will we be using
		switch (requests) {
		case exclude: 
		case include:
			free(filterlist->ports[i]);
			break;
		case filterip:
		case appexclude:
		case appinclude:
			for (j = 0; j < filterlist->arrindex[i]; j++)
				free(filterlist->strings[i][j]);
			free(filterlist->strings[i]);
			break;
		case report:
		case list:
		default:
			break;
		}
	}
	free(filterlist->mask);
	free(filterlist);
	free(message); // free the json string

	Cleanup:
	// free the inbound jason object and token
	json_object_put(json_in);
	json_tokener_free(tok);

	return NULL;
}

void usage(void) {
	printf ("Insight Web10G client. Version: %3.2f\n", INSIGHT_VERSION);
	printf ("\t-h this help screen\n");
	printf ("\t-p listen port\n");
	printf ("\t-g path to geoip database\n");
	printf ("\t-d print debug statements to stdout\n");
	printf ("\t-j print outbound json string to stdout\n");
}

// once we have an incoming message on the connection do something 
// with it. for the most part we only expect text. Anything else
// throws a non-fatal exception
int
onmessage(libwebsock_client_state *state, libwebsock_message *msg)
{
	switch (msg->opcode) {
	case WS_OPCODE_TEXT:{
		analyze_inbound(state, msg);
		break;
	}
	default:
		//fprintf(stderr, "Unknown inbound request (Non-text input): %d\n", msg->opcode);
		break;
	}
	return 0;
}

// confirm that the connection has been established
int
onopen(libwebsock_client_state *state)
{
	fprintf(stderr, "Incoming connection: %d\n", state->sockfd);
	return 0;
}

// when we close do we need to do anything? Probably not. 
int
onclose(libwebsock_client_state *state)
{
	fprintf(stderr, "Closed connection: %d\n", state->sockfd);
	return 0;
}

int main(int argc, char *argv[])
{
	libwebsock_context *wssocket = NULL;
	char* ip = "127.0.0.1";
	char* port = "9000";
	char* geoippath = "/usr/local/share/GeoIP/GeoLite2-City.mmdb";
	int opt;
	mysql_library_init(0, NULL, NULL);

	while ((opt = getopt(argc, argv, "hp:g:b:dj")) != -1) {
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
		case 'b':
			ip = optarg;
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
			break;
		}
	}

	// init and bind the websock client
	wssocket = libwebsock_init();
	if (wssocket == NULL ) {
		fprintf(stderr, "Error during libwebsock_init.\n");
		exit(1);
	}
	libwebsock_bind(wssocket, ip, port);
	fprintf(stderr, "Insight listening on port %s\n", port);

	// initialize the geoip database
	int status = MMDB_open(geoippath, MMDB_MODE_MMAP, &geoipdb);
	if (MMDB_SUCCESS != status) {
		fprintf(stderr, "Can't open geoIP database\n");
		return 0;
	}

	printf ("geoIP database opened and ready\n");

	// define the callbacks: what we do when we have to do something
	wssocket->onmessage = onmessage; // for each incoming message 
	wssocket->onopen = onopen; // this is for any connecting thread
	wssocket->onclose = onclose; // shut it down. shut it all down. 

	libwebsock_wait(wssocket); // And now we wait. Like a tick. 

	//perform any cleanup here.
	fprintf(stderr, "Exiting Insight.\n");
	MMDB_close(&geoipdb);
	mysql_library_end();
	return 0;
}
