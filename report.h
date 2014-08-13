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

#ifndef REPORT_H
#define REPORT_H

#include "config.h"
#ifdef HAVE_LIBJSONC
#include <json-c/json.h>
#else
#include <json/json.h>
#endif
#include "main.h"
#include "mysql/mysql.h"
#include "mysql/my_global.h"
#include "string-funcs.h"
#include "debug.h"

typedef struct jsondata {
	char *key_arr[150];
	char *val_arr[150];
	char *daddr;
	char *saddr;
	char *dport;
	char *sport;
	char *application;
	int idx;
} jsondata;


typedef struct reportinfo {
	int cid;
	char *uri;
	int port;
	char *db;
	char *dbname;
	char *dbpass;
	char *nocemail;
	char *fname;
	char *lname;
	char *email;
	char *phone;
	char *institution;
} reportinfo;

int report_create_from_json_string(char *);
int report_parse (json_object *, reportinfo *);
void report_sanitize (reportinfo *, MYSQL *);
void report_free (reportinfo *);
int report_sql (reportinfo *, char *);
int report_create_data_query (char *, char **, char **);
void report_parse_json_object (json_object *, jsondata *);
void report_parse_json_array (json_object *, char *, jsondata * );
#endif
