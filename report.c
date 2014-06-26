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
#include <json/json.h>
#include <stdio.h>
#include <string.h>
#include "report.h"

void report_parse (json_object *incoming, reportinfo *report) {
	json_object_object_foreach (incoming, key, val) {
		//	printf ("key: %s - %d\n", key, strlen(key));
		if (strcmp(key, "cid") == 0 ) {
			report->cid = json_object_get_int(val);
			continue;
		} 
		if (strcmp(key, "uri") == 0) {
			report->uri = malloc((strlen(json_object_get_string(val))+1) * sizeof(char));
			strcpy(report->uri, json_object_get_string(val));
			continue;
		}
		if (strcmp(key, "port") == 0) {
			report->port = json_object_get_int(val);
			continue;
		}
		if (strcmp(key, "fname") == 0) {
			report->fname = malloc((strlen(json_object_get_string(val))+1) * sizeof(char));
			strcpy(report->fname, json_object_get_string(val));
			continue;
		}
		if (strcmp(key, "lname") == 0) {
			report->lname = malloc((strlen(json_object_get_string(val))+1) * sizeof(char));
			strcpy(report->lname, json_object_get_string(val));
			continue;
		}
		if (strcmp(key, "email") == 0) {
			report->email = malloc((strlen(json_object_get_string(val))+1) * sizeof(char));
			strcpy(report->email, json_object_get_string(val));
			continue;
		}
		if (strcmp(key, "dbname") == 0) {
			report->dbname = malloc((strlen(json_object_get_string(val))+1) * sizeof(char));
			strcpy(report->dbname, json_object_get_string(val));
			continue;
		}
		if (strcmp(key, "dbpass") == 0) {
			report->dbpass = malloc((strlen(json_object_get_string(val))+1) * sizeof(char));
			strcpy(report->dbpass, json_object_get_string(val));
			continue;
		}
		if (strcmp(key, "nocemail") == 0 ) {
			report->nocemail = malloc((strlen(json_object_get_string(val))+1) * sizeof(char));
			strcpy(report->nocemail, json_object_get_string(val));
			continue;
		}
		if (strcmp(key, "institution") == 0) {
			report->institution = malloc((strlen(json_object_get_string(val))+1) * sizeof(char));
			strcpy(report->institution, json_object_get_string(val));
			continue;
		}
	}
	return;
}

void report_free(reportinfo *report_t) {
	free(report_t->uri);
	free(report_t->dbname);
	free(report_t->dbpass);
	free(report_t->fname);
	free(report_t->lname);
	free(report_t->nocemail);
	free(report_t->email);
	free(report_t->institution);
	free(report_t);
	return;
}

void report_dump(reportinfo *report_t) {
	  printf("cid: %d\n", report_t->cid);
	  printf("port: %d\n", report_t->port);
	  printf("uri : %s\n", report_t->uri);
	  printf("dbname : %s\n", report_t->dbname);
	  printf("dbpass : %s\n", report_t->dbpass);
	  printf("fname : %s\n", report_t->fname);
	  printf("lname : %s\n", report_t->lname);
	  printf("nocemail : %s\n", report_t->nocemail);
	  printf("email : %s\n", report_t->email);
	  printf("institution : %s\n", report_t->institution);
	  return;
}

// wraper function to take the incoming json string and report struct
// and build a valid sql query, submit it to the db, and alert the NOC
int report_sql(reportinfo *report_t, char *message) {
  

}

