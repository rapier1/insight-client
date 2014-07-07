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


typedef enum RequestTypes {
	list,
	exclude,
	include,
	filterip,
	appexclude,
	appinclude,
	report
} RequestTypes;

typedef struct CommandList {
	char *commands[10];
	char *options[10];
	char *mask;
	int maxindex;
} CommandList;

typedef struct FilterList {
	char *commands[10]; //the command
	char **strings[10]; // either app name or ips
	int *ports[10]; // port array
	int arrindex[10]; // number of elements in the strings or ports array
	int maxindex;
	int reportcid;
	char *mask;
} FilterList;

int includePort (int, int, int [], int);
int excludePort (int, int, int [], int);
int filterIPs( char*, char*, char**, int);
int parsePorts(struct FilterList *, char *, int);
int parseIPs(struct FilterList *, char *, int);
void json_parse_array (json_object *, char *, struct CommandList *);
void json_parse (json_object *, struct CommandList *);
void parse_comlist (struct CommandList *, struct FilterList *);
RequestTypes parse_string_to_enum( const char * );
