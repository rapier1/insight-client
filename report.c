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

#include <stdio.h>
#include <string.h>
#include "report.h"

/* add a cid to the struct we use to monitor the connections we are reporting on */
int report_add_cid (reportinfo *report, struct reports_ll **report_list_head) {
	printf ("in report_add_cid\n");
	struct reports_ll *curr;
	curr = *report_list_head;
	// initialize the list if we don't have anything in it already
	if (curr == NULL) {
		printf ("curr address is %p\n", curr);
		curr = (struct reports_ll *)malloc(sizeof(struct reports_ll));
		curr->report = report;
		curr->next = NULL;
		*report_list_head = curr;
		printf ("creating initial linked list at %p\n", report_list_head);
		return 1;
	}
	// go to the end of the list
	while (curr->next != NULL)
		curr = curr->next;
	curr->next = (struct reports_ll *)malloc(sizeof(struct reports_ll));
	curr->next->report = report;
	curr->next->next = NULL;
	printf ("curr %p curr->next %p\n", curr, curr->next);
	return 1;
}


/* remove the cid from the struct we are using to hold the connections we are
 reporting on */
int report_del_cid (int cid, reports_ll **report_list_head) {
	struct reports_ll *curr = *report_list_head;
	struct reports_ll *prev = NULL;
	struct reports_ll *next = NULL;
	struct reports_ll *del = NULL;
	if (curr == NULL) // nothing to delete
		return 1;
	while (curr != NULL) {
		next = curr->next;
		printf ("next is %p\n", next);
		if (curr->report->cid == cid) {
			printf ("found cid in linked list\n");
			if (prev == NULL) // we are deleting the head of the list
				*report_list_head = next;
			else
				prev->next = next;
			del = curr;
		}
		printf ("prev1 %p, curr1 %p, next1 %p\n", prev, curr, next);
		prev = curr;
		curr = next;
		printf ("prev2 %p, curr2 %p, next2 %p\n", prev, curr, next);
	}
	if (del != NULL) {
		printf ("freeing report and node at %p\n", del);
		report_free(del->report);
		free(del);
	}
	return 1;
}


int report_parse (json_object *json_in, reportinfo *report) {
	struct timeval time_s;
	// initialize the report structure
	// if they don't pass something we need to be able to know
	report->cid = 0;
	report->port = 0;
	report->persist = 0;
	report->interval = 0;
	report->uri = "\0";
	report->lname = "\0";
	report->fname = "\0";
	report->email = "\0";
	report->db = "\0";
	report->dbname = "\0";
	report->dbpass = "\0";
	report->nocemail = "\0";
	report->institution = "\0";
	report->phone = "\0";
	gettimeofday(&time_s, NULL);
	report->update_time = time_s.tv_sec; // used to determine when to send reports to the DB
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
		if (strcmp(key, "interval") == 0 ) {
			report->interval = json_object_get_int(val);
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
		if (strcmp(key, "persist") == 0) {
			report->persist = json_object_get_int(val);
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
			report->db = malloc((strlen(json_object_get_string(val))+1) * sizeof(char));
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

void json_result_free(jsondata *result) {
	int i;
	for (i = 0; i< result->idx; i++) {
		free(result->key_arr[i]);
		free(result->val_arr[i]);
	}
	free(result->daddr);
	free(result->saddr);
	free(result->dport);
	free(result->sport);
	free(result->application);
	free(result);
}

void report_dump (reportinfo *report) {
	printf("cid %d\n", report->cid);
	printf("port %d\n", report->port);
	printf("uri %s\n", report->uri);
	printf("db %s\n", report->db);
	printf("dbname %s\n", report->dbname);
	printf("dbpass %s\n", report->dbpass);
	printf("fname %s\n", report->fname);
	printf("lname %s\n", report->lname);
	printf("nocemail %s\n", report->nocemail);
	printf("email %s\n", report->email);
	printf("phone %s\n", report->phone);
	printf("institution %s\n", report->institution);
}

void report_sanitize(reportinfo *report, MYSQL *con) {
	char *from;
	//report_dump(report);
	from = report->uri;
	mysql_real_escape_string(con, report->uri, from, strlen(from));
	from = report->db;
	mysql_real_escape_string(con, report->db, from, strlen(from));
	from = report->dbname;
	mysql_real_escape_string(con, report->dbname, from, strlen(from));
	from = report->dbpass;
	mysql_real_escape_string(con, report->dbpass, from, strlen(from));
	from = report->fname;
	mysql_real_escape_string(con, report->fname, from, strlen(from));
	from = report->lname;
	mysql_real_escape_string(con, report->lname, from, strlen(from));
	from = report->nocemail;
	mysql_real_escape_string(con, report->nocemail, from, strlen(from));
	from = report->email;
	mysql_real_escape_string(con, report->email, from, strlen(from));
	from = report->phone;
	mysql_real_escape_string(con, report->phone, from, strlen(from));
	from = report->institution;
	mysql_real_escape_string(con, report->institution, from, strlen(from));
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
	char query[8092]; // likely an excessive size
	char uid[11];
	char fid[11];
	char sid[11];
	char rid[11];
	char timestamp[20];
	
	if (con == NULL) {
		fprintf(stderr, "Could not initialize MySql");
		return -1;
	}

	// open the connection
	if (mysql_real_connect(con, report->uri, report->dbname, 
			       report->dbpass, report->db, 
			       report->port, NULL, 0) == NULL) {
		//error connecting to database
		fprintf(stderr, "%s\n", mysql_error(con));
		return -1;
	}

	// we have a valid connection now we want to make sure all of the user input
	// is properly sanitized by running the user input through an string escape function
	report_sanitize(report, con);

	// check to see if we have an existing entry for that email address
	snprintf(query, strlen(report->email) + 50, "SELECT uid FROM userinfo WHERE uemail='%s'", report->email); 
	log_debug("USER Query: %s", query); 
	if (mysql_real_query(con, query, (unsigned int)strlen(query)) != 0) {
		//error from statement
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		return -1;
	} else {
		results = mysql_store_result(con);
		if (mysql_num_rows(results) == 0) {
			//user does not exist so insert them into the database
			MYSQL_RES *uid_result;
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

			// now grab the new users UID
			strcpy(query, "SELECT LAST_INSERT_ID() FROM userinfo");
			log_debug("New user UID Query: %s", query); 
			if (mysql_real_query(con, query, (unsigned int)strlen(query)) != 0) {
				// error
				fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
				return -1;
			}
			uid_result = mysql_store_result(con);
			if ((mysql_num_rows(uid_result)) == 0) {
				mysql_free_result(results);
				printf("Could not get new user User Id\n");
				return -1;
			}
			row = mysql_fetch_row(uid_result);
			strcpy(uid, row[0]);
			mysql_free_result(uid_result);
		} else {
			// they exist so get a user id from the first query
			row = mysql_fetch_row(results);
			strcpy(uid, row[0]);
		}
		mysql_free_result(results);
	}

	// now we have to parse the incoming message structure and build a valid SQL query from it.
	// no problem right? WRONG!! The message is in char format and for ease of use we need to
	// convert it back into a JSON object, step through the object extracting keys into one char buffer
	// values into another char buffer then concatonate them into a query string.
	report_create_data_query (message, &data_query_str, &flow_query_str);
	
	//we know have a data query string and a flow query string. We can insert those into the
	// database.
	strcpy(query, data_query_str);
	log_debug("Data Query: %s", query);
	if (mysql_real_query(con, query, (unsigned int)strlen(query)) != 0) {
		//error from statement
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		free(data_query_str);
		free(flow_query_str);
		return -1;
	}
	free (data_query_str);

	// get the sid by querying for the last incremented autoinc from this client
	strcpy(query, "SELECT LAST_INSERT_ID() FROM data"); 
	log_debug("SID Query: %s", query); 
	if (mysql_real_query(con, query, (unsigned int)strlen(query)) != 0) {
		// error
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		free(flow_query_str);
		return -1;
	}

	results = mysql_store_result(con);
	if ((mysql_num_rows(results)) == 0) {
		mysql_free_result(results);
		printf("Could not get Session Id\n");
		return -1;
	}
	row = mysql_fetch_row(results);
	strcpy(sid, row[0]);
	mysql_free_result(results);

	// insert the flow data
	strcpy(query, flow_query_str);
	log_debug("Flow Query: %s", query);
	if (mysql_real_query(con, query, (unsigned int)strlen(query)) != 0) {
		//error from statement
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		free(flow_query_str);
		return -1;
	}
	free(flow_query_str); 

	// get the fid by querying for the last incremented autoinc from this client
	strcpy(query, "SELECT LAST_INSERT_ID() FROM flow"); 
	log_debug("FID Query: %s", query); 
	if (mysql_real_query(con, query, (unsigned int)strlen(query)) != 0) {
		// error
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		return -1;
	}
	results = mysql_store_result(con);
	if ((mysql_num_rows(results)) == 0) {
		mysql_free_result(results);
		printf("Could not get Flow Id\n");
		return -1;
	}
	row = mysql_fetch_row(results);
	strcpy(fid, row[0]);
	mysql_free_result(results);
	
	// now get the timestamp
	snprintf(query, strlen(sid) + 35, "SELECT time FROM data WHERE sid=%s", sid); 
	log_debug("Time Query: %s", query);
	if (mysql_real_query(con, query, (unsigned int)strlen(query)) != 0) {
		// error
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		return -1;
	}
	results = mysql_store_result(con);
	if ((mysql_num_rows(results)) == 0) {
		mysql_free_result(results);
		printf("Could not get Timestamp\n");
		return -1;
	}
	row = mysql_fetch_row(results);
	strcpy(timestamp, row[0]);
	mysql_free_result(results);

	// now we populate the report table so we can get the report id we need for the
	// flow data
	snprintf (query, (strlen(uid)+strlen(sid)+strlen(timestamp)+55), 
		  "INSERT INTO report ( uid, sid, ts_stat) VALUES (%s, %s, %s)", 
		  uid, sid, timestamp); 
	log_debug("Report Query: %s", query);
	if (mysql_real_query(con, query, (unsigned int)strlen(query)) != 0) {
		// error
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		return -1;
	}
	strcpy(query, "SELECT LAST_INSERT_ID() FROM report"); 
	log_debug("RID Query: %s", query); 
	if (mysql_query(con, query) != 0) {
		// error
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		return -1;
	}
	results = mysql_store_result(con);
	if ((mysql_num_rows(results)) == 0) {
		mysql_free_result(results);
		printf("Could not get Report ID\n");
		return -1;
	}
	row = mysql_fetch_row(results);
	if (row[0] == NULL) {
		printf ("No report ID\n");
		return -1;
	}
	strcpy(rid, row[0]);
	mysql_free_result(results);

	// now update the flow table with the report id
	snprintf (query,(strlen(rid)+strlen(fid)+40), "UPDATE flow SET rid='%s' WHERE fid=%s", rid, fid); 
	log_debug("Update Flow Query: %s", query); 
	if (mysql_real_query(con, query, (unsigned int)strlen(query)) != 0) {
		// error
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		return -1;
	}

	// and also update the data table with flow id
	snprintf(query, (strlen(fid)+strlen(sid)+40), "UPDATE data SET fid='%s' WHERE sid=%s", fid, sid);
	log_debug("Data Update Query: %s", query); 
	if (mysql_real_query(con, query, (unsigned int)strlen(query)) != 0) {
		// error
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		return -1;
	}	// get the sid by querying for the last incremented autoinc from this client
	strcpy(query, "SELECT LAST_INSERT_ID() FROM data"); 
	log_debug("SID Query: %s", query); 
	if (mysql_real_query(con, query, (unsigned int)strlen(query)) != 0) {
		// error
		fprintf(stderr, "%s\n%s\n", mysql_error(con), query);
		free(flow_query_str);
		return -1;
	}

	results = mysql_store_result(con);
	if ((mysql_num_rows(results)) == 0) {
		mysql_free_result(results);
		printf("Could not get Session Id\n");
		return -1;
	}
	row = mysql_fetch_row(results);
	strcpy(sid, row[0]);
	mysql_free_result(results);

	mysql_close(con); // free the mysql connection
	// this is in a threaded environment and mysql_init 
	// is calling a thread that we must explicitly close
	mysql_thread_end(); 
	// that should be all of the database foo. 
	return 1;
}

int report_create_data_query (char* request, char** dq_string, char** fq_string) {
	json_tokener *tok = json_tokener_new();
	json_object *json_in = NULL;
	jsondata *json_result = NULL;
	enum json_tokener_error jerr;
	char *key_str = NULL;
	char *val_str = NULL;
	char *delim = ", ";

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
	// parse the object and place the tuple information in fq_string and the 
	// data into dq_string
	json_result = malloc(sizeof(jsondata));
	json_result->idx = 0;
	report_parse_json_object(json_in, json_result);

	if (json_result->idx == 0) {// problem generating the data array
		json_object_put(json_in);
		json_tokener_free(tok);		
		return -1;
	}

	// take the arrays we've assembled and turn them in delimited strings
	join_strings(&key_str, json_result->key_arr, delim, json_result->idx);
	join_strings(&val_str, json_result->val_arr, delim, json_result->idx);
	*dq_string = malloc((strlen(key_str) + strlen(val_str) + 35) * sizeof(char));
	*fq_string = malloc(((strlen(json_result->daddr) + strlen(json_result->dport) +
			      strlen(json_result->saddr) + strlen(json_result->sport) +
			      strlen(json_result->application) + 95)) * sizeof(char));
	sprintf(*dq_string, "INSERT INTO data (%s) VALUES (%s)", key_str, val_str);
	sprintf(*fq_string,
		"INSERT INTO flow (daddr, dport, saddr, sport, application) VALUES ('%s', '%s', '%s', '%s', '%s')",
		json_result->daddr, json_result->dport, json_result->saddr, 
		json_result->sport, json_result->application);
	log_debug("%s", *dq_string);
	log_debug("%s", *fq_string);

	// free everything up we no longer need
	free(key_str);
	free(val_str);
	json_result_free(json_result);
	json_object_put(json_in);
	json_tokener_free(tok);
	return 1;
}

void report_parse_json_array (json_object *json_in, char* key, jsondata* json_result) {
	json_object *jarray = json_in;
	
	if(key) {
		json_object_object_get_ex(json_in, key, &jarray); /*Getting the array if it is a key value pair*/
	}
	
	int arraylen = json_object_array_length(jarray); /*Getting the length of the array*/
	int i;
	json_object * jvalue;

	for (i=0; i< arraylen; i++){
		jvalue = json_object_array_get_idx(jarray, i); /*Getting the array element at position i*/
		report_parse_json_object(jvalue, json_result);
	}
}

// recursive function to step through the json object and find the data we
// care about. Similar to routine in parse.c but getting different data
void report_parse_json_object (json_object *json_in, jsondata *json_result) {
	enum json_type type;
	char value[50];
	json_object *jobj_in = NULL;
	
	json_object_object_foreach(json_in, key, val) { /*Passing through every element*/
		type = json_object_get_type(val);
		switch (type) {
		case json_type_null:
		case json_type_boolean:
		case json_type_double:
		case json_type_int:
		case json_type_string:
			if (strcmp(key, "SrcIP") == 0) {
				json_result->saddr= strdup((char *)json_object_get_string(val));
				log_debug("SRCIP = %s", json_result->saddr);
			} else if (strcmp(key, "DestIP") == 0) {
				json_result->daddr = strdup((char *)json_object_get_string(val));
				log_debug("DESTIP = %s", json_result->saddr);
			} else if (strcmp(key, "SrcPort") == 0) {
				json_result->sport = strdup((char *)json_object_get_string(val));
				log_debug("SRCPORT = %s", json_result->sport);
			} else if (strcmp(key, "DestPort") == 0) {
				json_result->dport = strdup((char *)json_object_get_string(val));
				log_debug("DESTPORT = %s", json_result->dport);
			} else if (strcmp(key, "Application") == 0) {
				json_result->application = strdup((char *)json_object_get_string(val));
				log_debug("APP = %s", json_result->application);
			} else {
				// don't want either of these in the final data
				// may change our mind though
				if (strcmp(key, "lat") == 0)
					continue;
				if (strcmp(key, "long") == 0)
					continue;
				sprintf(value, "'%s'", (char *)json_object_get_string(val));
				json_result->key_arr[json_result->idx] = malloc(strlen(key)+1 * sizeof(char));
				strcpy(json_result->key_arr[json_result->idx], key);
				json_result->val_arr[json_result->idx] = malloc(strlen(value)+1 * sizeof(char));
				strcpy(json_result->val_arr[json_result->idx], value);
				json_result->idx++;
			}
			break;
		case json_type_array:
			report_parse_json_array(json_in, key, json_result);
			break;
		case json_type_object:
			json_object_object_get_ex(json_in, key, &jobj_in);
			report_parse_json_object(jobj_in, json_result);
			break;
		}
	}
}


// basically takes the incoming string and runs it through the rest of the functions to 
// create the report and send it. 
int report_create_from_json_string (char *incoming, reports_ll **report_list_head) {
	struct reportinfo *report;
	json_tokener *tok = json_tokener_new();
	json_object *json_in = NULL;
	enum json_tokener_error jerr;
	int myerr = 0;

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
		json_tokener_free(tok);
		fprintf(stderr, "Poorly formed JSON object. Check for extraneous characters.");
		return -1;
	}
	report = malloc(sizeof(struct reportinfo));
	if (report_parse(json_in, report) != 1) {
		// error here
		log_debug("Could not parse the json object in report_parse");
		report_free(report);
		json_object_put(json_in);
		json_tokener_free(tok);
		return -1;
	}
	
	printf ("report->persist is %d\n", report->persist);

	// add the report to the linked list
	if (report->persist > 0) {
		if (report_add_cid(report, report_list_head))
			// need to send the initial report
			myerr = report_execute (report);
	}
	// remove the report from the linked list
	if (report->persist < 0) 
		myerr = report_del_cid(report->cid, report_list_head);
	// they only want a single report
	if (!report->persist || report->persist == 0) {
		myerr = report_execute(report);
		report_free(report);
	}

	json_object_put(json_in);
	json_tokener_free(tok);
	return myerr;
}



int report_execute (struct reportinfo *report) {
	int myerr = 0;
	struct FilterList *filterlist;
	char *message;

	// filterlist needs the report command, cid, mask, and index size (always 1)
	filterlist = malloc(sizeof(struct FilterList));
	filterlist->commands[0] = strdup("report");
	filterlist->reportcid = report->cid;
	filterlist->mask = strdup("fffffffff,3ffffff,7ffffffffff,fff,f,3");
	filterlist->maxindex = 1;

	// get the flow data we are sending to the NOC
	get_connection_data(&message, filterlist);
	log_debug("Message: %s", message);

	// an empty message (created when they delay submitting a report until after the
	// cid disappears) is 18 characters long. We expect the message to be closer to 
	// 1k long so if it's tiny then there is a problem. 
	if (strlen(message) < 20) { 
		// the cid expired 
		log_debug("Empty Message in report_create_from_json_string");
		myerr = -1;
		goto Cleanup;
	}
	if (report_sql(report, message) != 1) {
		//error here
		log_debug("report_sql returned an error");
		myerr = -1;
		goto Cleanup;
	}

	myerr = 1;
Cleanup:
	free(filterlist->commands[0]);
	free(filterlist->mask);
	free(filterlist);
	free(message);
	return myerr;
}
