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

// take a string and see if there is CSV list of ports we 
// export the resulting array for use elsewhere
int parsePorts (int ports[], char *inbound) {
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
	for ( i = 0; i < mynum; i++ ) {
		// convert to integer and copy to our array
		ports[i] = atoi((const char*)split[i]);
	}
	free(split);
	free(strCpy);
	return mynum;
}

int parseIPs(char** ips, char *inbound) {
	char* strCpy;
        char** split; // array for the results
	int mynum; // number of elements in the returned array
        int i;

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
			strcpy(ips[i], noquotes(strip(split[i])));
		}
        free(split);
        free(strCpy);
        return mynum;
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
	int i;
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
	for (i = 0; i <= comindex; i++) {
	  if (comlist->options[i] == NULL) {
	    printf("empty option\n");
	    comlist->options[i] = malloc(1  * sizeof(char));
	    strcpy(comlist->options[i], empty);
	  }
	  if (comlist->commands[i] == NULL) {
	    printf("empty command\n");
	    comlist->commands[i] = malloc(1  * sizeof(char));
	    strcpy(comlist->commands[i], empty);
	  }
	}
	if (comlist->mask == NULL) {
	  printf("empty mask\n");
	  comlist->mask = malloc(1 *sizeof(char));
	  strcpy(comlist->mask, empty);
	}
} 
