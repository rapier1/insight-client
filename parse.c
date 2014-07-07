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

/* this file provides the routines for handling incoming requests
 * from the websocket and turning those request, when necessary, into 
 * structures needed to filter the tcpdata
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scripts.h"
#include "string-funcs.h"
#include <json-c/json.h>
#include "parse.h"

int comindex = 0;

// if the dest port or source port exists in the array of ports
// then return a 0. This sets the flag to zero and ensures that the
// tcpdata is not skipped
int includePort (int sport, int dport, int ports[], int index) {
	int i;
	for (i = 0; i < index; i++) {
		if (sport == ports[i]) {
			return 0;
		} 
		if (dport == ports[i]) {
			return 0;
		}
	}
	return 1;
}

// Same as above but if the tuples port is in the list then skip it. 
int excludePort (int sport, int dport, int ports[], int index) {
	int i;
	for (i = 0; i < index; i++) {
		if (sport == ports[i] || dport == ports[i]) {
			return ports[i];
		}
	}
	return 0;
}

// only report on IPs passed to us. This does a string comparison
// we can later expand this to do filtering on CIDR blocks and the like. 
int filterIPs( char* local, char* remote, char** ips, int index) {
	int i;
	for (i = 0; i < index; i++) {
		if (strcmp(local, ips[i]) == 0 ||
		    strcmp(remote, ips[i]) == 0)
			return 0;
	}
	return 1;
}

void json_parse_array( json_object *jobj, char *key, struct CommandList *comlist) {
	void json_parse(json_object * jobj, struct CommandList *comlist); /*Forward Declaration*/
	json_object *jarray = jobj; /*Simply get the array*/
	if(key) {
		json_object_object_get_ex(jobj, key, &jarray); /*Getting the array if it is a key value pair*/
	}
	
	int arraylen = json_object_array_length(jarray); /*Getting the length of the array*/
	int i;
	json_object * jvalue;
	
	for (i=0; i< arraylen; i++){
		jvalue = json_object_array_get_idx(jarray, i); /*Getting the array element at position i*/
		json_parse(jvalue, comlist);
	}
}


/*Parsing the json object*/
void json_parse(json_object * jobj, struct CommandList *comlist) {
	char * command;
	char * options;
	char * mask;
	int incflag = 0;
	int i = 0;
	char * empty = "\0";
	enum json_type type;
	
	json_object_object_foreach(jobj, key, val) { /*Passing through every array element*/
		type = json_object_get_type(val);
		if(strcmp(key, "command") == 0) {
			command = (char *)json_object_get_string(val);
			comlist->commands[comindex] = malloc((strlen(command)+1) * sizeof(char));
			strcpy(comlist->commands[comindex], command);
			//we need in increment i but only when we see a command
			//and an option so if incflag = 2 then we increment i
			incflag++;
		}
		if(strcmp(key, "options") == 0) {
			options = (char *)json_object_get_string(val);
			comlist->options[comindex] = malloc((strlen(options)+1) * sizeof(char));
			strcpy(comlist->options[comindex], options);
			incflag++;
		}
		if(strcmp(key, "mask") == 0) {
			mask = (char *)json_object_get_string(val);
			comlist->mask = malloc((strlen(mask)+1) * sizeof(char));
			strcpy(comlist->mask, mask);
		}
		if (incflag == 2) {
			incflag = 0;
			comindex++;
		}
		switch (type) {
		case json_type_null:
		case json_type_boolean: 
		case json_type_double: 
		case json_type_int: 
		case json_type_string:
			break; 
		case json_type_object: 
			json_object_object_get_ex(jobj, key, &jobj);
			json_parse(jobj, comlist); 
			break;
		case json_type_array: 
			json_parse_array(jobj, key, comlist);
			break;
		}
	}
	// we need to make sure every element of comlist is properly initialized
	// there are cases in which we may have a command with no options or where
	// mask is not set. As such we want to put null strings in there
	for (i = 0; i < comindex; i++) {
		if (comlist->options[i] == NULL) {
			comlist->options[i] = malloc(1  * sizeof(char));
			strcpy(comlist->options[i], empty);
		}
		if (comlist->commands[i] == NULL) {
			comlist->commands[i] = malloc(1  * sizeof(char));
			strcpy(comlist->commands[i], empty);
		}
	}
	if (comlist->mask == NULL) {
		comlist->mask = malloc(1 *sizeof(char));
		strcpy(comlist->mask, empty);
	}
	comlist->maxindex = comindex; // how many array items do we have? 
} 

// take a string and see if there is CSV list of ports we 
// export the resulting array for use elsewhere
int parsePorts(struct FilterList *filterlist, char *inbound, int loc) {
	char* strCpy;
	char** split; // array for the results
	int mynum; // number of elements in the returned array
	int i;
	
	// create a copy of the string
	strCpy = malloc( (strlen(inbound)+1) * sizeof( *strCpy ) );
	strcpy( strCpy, inbound );

	split = str_split (strCpy, ',', &mynum);
	if ( split == NULL ) {
		// We didn't find the delimiter between the command and list of options
		// which is obviously odd but lets accept it for now
		return 0;
	}
	filterlist->ports[loc] = malloc(sizeof(*filterlist->ports));
	for ( i = 0; i < mynum; i++ ) {
		// convert to integer and copy to our array
		filterlist->ports[loc][i] = -1; // malloc(sizeof(*filterlist->ports[loc]));
		filterlist->ports[loc][i] = atoi((const char*)split[i]);
	}
	free(split);
	free(strCpy);
	return mynum;
}

int parseIPs(struct FilterList *filterlist, char *inbound, int loc) {
	char* strCpy;
        char** split; // array for the results
	int mynum; // number of elements in the returned array
        int i;
	
	filterlist->strings[loc] = malloc(10 * sizeof(*filterlist->strings[loc]));

	// remalloc strCpy to the size of split[1]
	strCpy = malloc((strlen(inbound)+1) * sizeof( *strCpy));
	// copy the contents (CSV list of ports) to a string
	strcpy(strCpy, inbound);
	split = str_split (strCpy, ',', &mynum);
	if ( split == NULL ) {
		// We didn't find the delimiter between the command and list of options
		// which is obviously odd but lets accept it for now
		return 0;
	}
	for ( i = 0; i < mynum; i++ ) {
		filterlist->strings[loc][i] = malloc((strlen(split[i])+1) * sizeof(char));
		strcpy(filterlist->strings[loc][i], noquotes(strip(split[i])));
	}
	free(split);
        free(strCpy);
        return mynum;
}

void parse_comlist (struct CommandList *comlist, struct FilterList *filterlist) {
	int max = (int)comlist->maxindex;
	char *mask = comlist->mask;
	int i;
	enum RequestTypes request;

	filterlist->maxindex = max;
	filterlist->mask = strdup(mask);
	for (i = 0; i < max; i++) {
		request = parse_string_to_enum(comlist->commands[i]);
		filterlist->commands[i] = strdup(comlist->commands[i]);
		if (request == -1) // couldn't map the command to the enum so ignore
			continue;
		switch (request) {
		case list:
			break;
		case exclude:
			filterlist->arrindex[i] = parsePorts(filterlist, comlist->options[i], i);
			break;
		case include:
			filterlist->arrindex[i] = parsePorts(filterlist, comlist->options[i], i);
			break;
		case filterip:
			filterlist->arrindex[i] = parseIPs(filterlist, comlist->options[i], i);
			break;
		case appexclude:
			break;
		case appinclude:
			break;
		case report:
			break;
		default:
			break;
		}
	}
}

/* take the incoming string and convert it to the corresponding 
 * enum in the specified struct. 
 */
RequestTypes parse_string_to_enum( const char *s ) {
    static struct {
        const char *s;
        RequestTypes e;
    } map[] = {
        { "list", list },
        { "exclude", exclude },
        { "include", include },
	{ "filterip", filterip },
	{ "appexclude", appexclude },
	{ "appinclude", appinclude },
	{ "report", report },
    };
    int i;
    for ( i = 0 ; i < sizeof(map)/sizeof(map[0]); i++ ) {
        if ( strcmp(s,map[i].s) == 0 ) {
            return map[i].e;
        }
    }
    return -1;
}
