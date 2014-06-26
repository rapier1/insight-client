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
