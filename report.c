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
#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include "report.h"
#include "main.h"
#include "mysql.h"
#include "my_global.h"
#include "string-funcs.h"
#include "debug.h"

int report_parse (json_object *json_in, reportinfo *report) {
	enum json_type type;
	json_object_object_foreach (json_in, key, val) {
		type = json_object_get_type(val);
		if (type == json_type_object) {
			// we need to be in the options object
			json_object_object_get_ex(json_in, key, &json_in);
			report_parse(json_in, report);
		}
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
		if (strcmp(key, "db") == 0) {
			report->dbname = malloc((strlen(json_object_get_string(val))+1) * sizeof(char));
			strcpy(report->db, json_object_get_string(val));
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
		if (strcmp(key, "phone") == 0) {
			report->phone = malloc((strlen(json_object_get_string(val))+1) * sizeof(char));
			strcpy(report->phone, json_object_get_string(val));
			continue;
		}
	}
	return 1;
}

void report_free(reportinfo *report) {
	free(report->uri);
	free(report->db);
	free(report->dbname);
	free(report->dbpass);
	free(report->fname);
	free(report->lname);
	free(report->nocemail);
	free(report->email);
	free(report->institution);
	free(report->phone);
	free(report);
	return;
}

void report_dump(reportinfo *report) {
	  printf("cid: %d\n", report->cid);
	  printf("port: %d\n", report->port);
	  printf("uri : %s\n", report->uri);
	  printf("db : %s\n", report->db);
	  printf("dbname : %s\n", report->dbname);
	  printf("dbpass : %s\n", report->dbpass);
	  printf("fname : %s\n", report->fname);
	  printf("lname : %s\n", report->lname);
	  printf("nocemail : %s\n", report->nocemail);
	  printf("email : %s\n", report->email);
	  printf("phone : %s\n", report->phone);
	  printf("institution : %s\n", report->institution);
	  return;
}

// function to take the incoming json string and report struct
// and build a valid sql query, submit it to the db, and alert the NOC
int report_sql(reportinfo* report, char* message) {
	MYSQL *con = mysql_init(NULL);
	MYSQL_RES *results;
	MYSQL_ROW row;
	char *data_query_str;
	char *flow_query_str;
	char query[1024]; // likely an excessive size
	char uid[11];
	char fid[11];
	char sid[11];
	char rid[11];
	char timestamp[20];

	// open the connection
	if (mysql_real_connect(con, report->uri, report->dbname, 
			       report->dbpass, report->db, 
			       report->port, NULL, 0) == NULL) {
		//error connecting to database
		fprintf(stderr, "%s\n", mysql_error(con));
		return -1;
 	}
	// we have a valid connection
	// check to see if we have an existing entry for that email address
	sprintf(query, "SELECT uemail FROM userinfo WHERE uemail='%s'", report->email);
	log_debug("Query: %s", query);
	if (mysql_query(con, query) != 0) {
		//error from statement
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		return -1;
	} else {
		results = mysql_store_result(con);
		if (results == NULL) {
			//user does not exist so insert them into the database
			sprintf(query, 
				"INSERT INTO userinfo (uemail, ufname, ulname, uinstitution, uphone) VALUES ('%s', '%s', '%s', '%s', '%s')",
				 report->email,
				 report->fname,
				 report->lname,
				 report->institution,
				 report->phone);
			log_debug("Query: %s", query);
			if (mysql_query(con, query) != 0) {
				//error from statement
				fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
				return -1;
			}
		}
		mysql_free_result(results);
	}

	// okay, a user exists in the database, one way or the other, now
	// so get their uid so we can use it in the other tables
	sprintf(query, "SELECT uid FROM userinfo WHERE uemail='%s'", report->email);
	log_debug("Query: %s", query);
	if (mysql_query(con, query) != 0) {
		// error
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		return -1;
	} 
	results = mysql_store_result(con);
	row = mysql_fetch_row(results);
	mysql_free_result(results);
	strcpy(uid, row[0]);
	printf("UID %s\n", uid);

	// now we have to parse the incoming message structure and build a valid SQL query from it. 
	// no problem right? WRONG!! The message is in char format and for ease of use we need to 
	// convert it back into a JSON object, step through the object extracting keys into one char buffer
	// values into another char buffer then concatonate them into a query string. 
	report_create_data_query (message, &data_query_str, &flow_query_str);

	//we know have a data query string and a flow query string. We can insert those into the 
	// database. 
	log_debug("Data Query: %s", data_query_str);
	if (mysql_query(con, data_query_str) != 0) {
		//error from statement
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		free(data_query_str);
		free(flow_query_str);
		return -1;
	}
	free (data_query_str);

	log_debug("Flow Query: %s", flow_query_str);
	if (mysql_query(con, flow_query_str) != 0) {
		//error from statement
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		free(data_query_str);
		free(flow_query_str);
		return -1;
	}
	free(flow_query_str);


	// get the sid by querying for the last incremented autoinc from this client
	strcpy(query, "SELECT LAST_INSERT_ID() FROM data");
	log_debug("Query: %s", query);
	if (mysql_query(con, query) != 0) {
		// error
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		return -1;
	} 
	results = mysql_store_result(con);
	row = mysql_fetch_row(results);
	mysql_free_result(results);
	strcpy(sid, row[0]);

	// now get the timestamp
	sprintf(query, "SELECT time FROM data WHERE sid=%s", sid);
	log_debug("Query: %s", query);
	if (mysql_query(con, query) != 0) {
		// error
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		return -1;
	} 
	results = mysql_store_result(con);
	row = mysql_fetch_row(results);
	mysql_free_result(results);	
	strcpy(timestamp, row[0]);

	// now we populate the report table so we can get the report id we need for the 
	// flow data
	sprintf (query, "INSERT INTO report ( uid, sid, ts_stat) VALUES (%s, %s, %s)", 
		 uid, sid, timestamp);
	if (mysql_query(con, query) != 0) {
		// error
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		return -1;
	} 
	strcpy(query, "SELECT LAST_INSERT_ID() FROM report");
	log_debug("Query: %s", query);
	if (mysql_query(con, query) != 0) {
		// error
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		return -1;
	} 
	results = mysql_store_result(con);
	row = mysql_fetch_row(results);
	mysql_free_result(results);
	strcpy(rid, row[0]);	

	// get the fid by querying for the last incremented autoinc from this client
	strcpy(query, "SELECT LAST_INSERT_ID() FROM flow");
	log_debug("Query: %s", query);
	if (mysql_query(con, query) != 0) {
		// error
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		return -1;
	} 
	results = mysql_store_result(con);
	row = mysql_fetch_row(results);
	mysql_free_result(results);
	strcpy(fid, row[0]);
	
	// now update the flow table with the report id
	sprintf (query, "UPDATE flow rid=%s WHERE fid=%s", rid, fid);
	log_debug("Query: %s", query);
	if (mysql_query(con, query) != 0) {
		// error
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		return -1;
	} 	

	// and also update the data table with flow id
	sprintf(query, "UPDATE data fid=%s WHERE sid=%s", fid, sid);
	log_debug("Query: %s", query);
	if (mysql_query(con, query) != 0) {
		// error
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		return -1;
	} 	

	// that should be all of the database foo. 
	return 1;
}

int report_create_data_query (char* request, char** dq_string, char** fq_string) {
	json_tokener *tok = json_tokener_new();
	json_object *json_in = NULL;
	enum json_tokener_error jerr;

	// grab the incoming json string
	json_in = json_tokener_parse_ex(tok, request, strlen(request));
	while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);
	if (jerr != json_tokener_success)
	{
		// it doesn't seem to be a json object
		// at this point we'll just return null and wait for the next message
		fprintf(stderr, "Inbound JSON Error: %s\n", json_tokener_error_desc(jerr));
		return -1;
	}
	
	if (tok->char_offset < strlen(request)) // XXX shouldn't access internal fields
	{
		// this is when we get characters appended to the end of the json object
		// since this shouldn't be sent from the UI we'll just return null and wait for 
		// a new message
		fprintf(stderr, "Poorly formed JSON object. Check for extraneous characters.");
		return -1;
	}
	// we have a validated json_object assembled from our incoming character string
	// pasre the object and place the tuple information in fq_string and the 
	// data into dq_string
	if (report_parse_json_object(json_in, dq_string, fq_string) == 1)
	       return 1;	
	
	// some sort of error
	return -1;
}

int report_parse_json_array (json_object *json_in, char* key, char** dq_string, char** fq_string) {
	json_object *jarray = json_in;
	
	if(key) {
		json_object_object_get_ex(json_in, key, &jarray); /*Getting the array if it is a key value pair*/
	}
	
	int arraylen = json_object_array_length(jarray); /*Getting the length of the array*/
	int i;
	json_object * jvalue;
	
	for (i=0; i< arraylen; i++){
		jvalue = json_object_array_get_idx(jarray, i); /*Getting the array element at position i*/
		report_parse_json_object(jvalue, dq_string, fq_string);
	}
	return 1;
}

int report_parse_json_object (json_object *json_in, char** dq_string, char** fq_string) {
	enum json_type type;
	char **key_arr;
	char **val_arr;
	char *value;
	char *key_str;
	char *val_str;
	char *delim = ", ";
	char *daddr = NULL; 
	char *saddr = NULL; 
	char *dport = NULL;
	char *sport = NULL; 
	char *application = NULL;
	int idx = 0;
	int i = 0; 
	
	key_arr = malloc(sizeof(*key_arr));
	val_arr = malloc(sizeof(*val_arr));
	json_object_object_foreach(json_in, key, val) { /*Passing through every element*/
		type = json_object_get_type(val);
		if (strcmp(key, "DATA") != 0) {
			if (strcmp(key, "SrcIP") == 0) {
				saddr = (char *)json_object_get_string(val);
			} else if (strcmp(key, "DestIP") == 0) {
				daddr = (char *)json_object_get_string(val);
			} else if (strcmp(key, "SrcPort") == 0) {
				sport = (char *)json_object_get_string(val);
			} else if (strcmp(key, "DestPort") == 0) {
				dport = (char *)json_object_get_string(val);
			} else if (strcmp(key, "Application") == 0) {
				application = (char *)json_object_get_string(val);
			} else {
				value = (char *)json_object_get_string(val);
				key_arr[idx] = strdup(key);
				val_arr[idx] = strdup(value);
				idx++;
			}
		}
		switch (type) {
		case json_type_null:
		case json_type_boolean: 
		case json_type_double: 
		case json_type_int: 
		case json_type_string:
		case json_type_array: 
			report_parse_json_array(json_in, key, dq_string, fq_string);
			break; 
		case json_type_object: 
			json_object_object_get_ex(json_in, key, &json_in);
			report_parse_json_object(json_in, dq_string, fq_string); 
			break;
		}
	}
	key_str = join_strings(*key_arr, delim, i);	
	val_str = join_strings(*val_arr, delim, i);
	log_debug ("Key string: %s", key_str);
	log_debug ("Value string: %s", val_str)
	*dq_string = malloc((strlen(key_str) + strlen(val_str) + 40) * sizeof(char));
	*fq_string = malloc(((strlen(daddr) + strlen(dport) + 
			      strlen(saddr) + strlen(sport) + 
			      strlen(application) + 100)) * sizeof(char));
	sprintf(*dq_string, "INSERT INTO data (%s) VALUES (%s)", key_str, val_str);
	sprintf(*fq_string, 
		"INSERT INTO flow (daddr, dport, saddr, sport, application) VALUES ('%s', '%s', '%s', '%s', '%s')", 
		daddr, dport, saddr, sport, application);

        for (i = 0; i < idx; i++) {
		free(key_arr[i]);
		free(val_arr[i]);
	}
	free(key_arr);
	free(val_arr);
	return 1;
}


// basically takes the incoming string and runs it through the rest of the functions to 
// create the report and send it. 
int report_create_from_json_string (char *incoming) {
	struct reportinfo *report;
	struct FilterList *filterlist;
	char *message;
	json_tokener *tok = json_tokener_new();
	json_object *json_in = NULL;
	enum json_tokener_error jerr;

	json_in = json_tokener_parse_ex(tok, incoming, strlen(incoming));

	while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);
	if (jerr != json_tokener_success)
	{
		// it doesn't seem to be a json object
		// at this point we'll just return null and wait for the next message
		fprintf(stderr, "Inbound JSON Error: %s\n", json_tokener_error_desc(jerr));
		return -1;
	}
	
	if (tok->char_offset < strlen(incoming)) // XXX shouldn't access internal fields
	{
		// this is when we get characters appended to the end of the json object
		// since this shouldn't be sent from the UI we'll just return null and wait for 
		// a new message
		fprintf(stderr, "Poorly formed JSON object. Check for extraneous characters.");
		return -1;
	}
	report = malloc(sizeof(struct reportinfo));
	if (report_parse(json_in, report) != 1) {
		// error here
		log_debug("Could not parse the json object in report_parse");
		report_free(report);
		return -1;
	}
	// filterlist only needs a "report" command and the connection id
	filterlist = malloc(sizeof(struct FilterList));
	filterlist->commands[0] = strdup("report");
	filterlist->reportcid = report->cid;
	// get the flow data we are sending to the NOC
	get_connection_data(&message, filterlist);
	// TODO: i need to find out what the length of the message will
	// be if it doesn't return any data at all. It might be zero
	// but you need to check. 
	if (strlen(message) < 5) { 
		// the cid expired 
		log_debug("Empty Message in report_create_from_json_string");
		free(filterlist->commands);
		free(filterlist);
		report_free(report);
		free(message);
		return -1;
	}
	if (report_sql(report, message) != 1) {
		//error here
		log_debug("report_sql returned an error");
		free(filterlist->commands);
		free(filterlist);
		report_free(report);
		free(message);
		return -1;
	}
	free(filterlist->commands);
	free(filterlist);
	report_free(report);
	free(message);
	return 1;
}


