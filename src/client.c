/**
 * @file
 * @brief This file implements a "very" simple sample client.
 * 
 * The client connects to the server, running at SERVERHOST:SERVERPORT
 * and performs a number of storage_* operations. If there are errors,
 * the client exists.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "storage.h"

#include "utils.h"
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#define FILE_NAME "Population.text"
#define SERVER_HOST "localhost"
#define SERVER_PORT 2159
#define USERNAME "admin"
#define PASSWORD "dog4sale"
#define TABLE "census"
#define KEY "key1"
#define VALUE "Province val1, Population 1, Change 1, Rank 1"
#define PREDICATES "Population > 30000, Change > 5"
#define PERIOD_GET 50L
#define PERIOD_SET 500000L
#define PERIOD_QUERY 50000L
#define PERIOD_TRANSACTION 2100L
#define TOTAL_TRANSACTIONS 1500

// different client modes
void bulk_load();
int client_standard();
int client_repeat_get();
int client_repeat_set_single();
int client_repeat_query();
int client_repeat_transaction();
// helper: get two time's difference in microseconds
long get_time_diff(struct timeval before, struct timeval after);

// Connection and authentication status checks
int is_connected;
int is_authenticated;

// Helper functions
int get_string_input(char* prompt, char* dest, int length);
int get_int_input(char* promt, int* dest, int low_bound, int up_bound);
int status_check(int shoud_be_connected, int should_be_authenticated);



int main(int argc, char *argv[]) {
	int selection = -1;
	while (selection != 5) {
		// Display menu
		printf("\n");
		printf("> ----------------------------------\n");
		printf("> 0) Bulk load workload data        \n");
		printf("> 1) Standard                       \n");
		printf("> 2) Repeated get request           \n");
		printf("> 3) Repeated set request           \n");
		printf("> 4) Repeated query request         \n");
		printf("> 5) Repeated transactions          \n");
		printf("> 6) Exit                           \n");
		printf("> ----------------------------------\n");
		// Accept selection input
		get_int_input("> Please enter your selection:",&selection,0,6);
		// Execute the chosen client mode
		switch (selection) {
		case 0:
		{
			bulk_load();
			break;
		}
		case 1:
		{
			return client_standard();
			break;
		}
		case 2:
		{
			return client_repeat_get();
			break;
		}
		case 3:
		{
			return client_repeat_set_single();
			break;
		}
		case 4:
		{
			return client_repeat_query();
			break;
		}
		case 5:
		{
			return client_repeat_transaction();
			break;
		}
		case 6:
		{
			// do nothing
			break;
		}
		}
	}
	return 0;
}

void bulk_load() {
	// connect
	void* conn = storage_connect(SERVER_HOST,SERVER_PORT);
	// authenticate
	storage_auth(USERNAME,PASSWORD,conn);
	// bulk load
	char keys[MAX_RECORDS_PER_TABLE][MAX_KEY_LEN], values[MAX_RECORDS_PER_TABLE][MAX_VALUE_LEN];
	if (extract_kv_from_file(FILE_NAME,keys,values) != 0) {
		printf("> Failed\n");
		return;
	}
	// Issue storage_set with loop
	int k = 0;
	while (keys[k][0] != '\0' && values[k][0] != '\0') {
		struct storage_record r;
		strncpy(r.value, values[k], sizeof values[k]);
		r.metadata[0] = 0;
		storage_set(TABLE, keys[k], &r, conn);
		k++;
	}
	// disconnect
	storage_disconnect(conn);
}

int client_repeat_get() {
	// connect
	void* conn = storage_connect(SERVER_HOST,SERVER_PORT);
	// authenticate
	storage_auth(USERNAME,PASSWORD,conn);
	// set a data entry
	struct storage_record r1;
	strcpy(r1.value,VALUE);
	r1.metadata[0] = 0;
	storage_set(TABLE,KEY,&r1,conn);
	// repeatedly get a data entry
	while (1) {
		// get
		struct storage_record r2;
		struct timeval start_time, end_time;
		gettimeofday(&start_time, NULL);
		storage_get(TABLE,"CookshireEaton",&r2,conn);
		gettimeofday(&end_time, NULL);
		printf("Get command done: used %ld microseconds\n", get_time_diff(start_time,end_time));
		// delay for a period
		struct timeval mark_time, now_time;
		gettimeofday(&mark_time, NULL);
		do {
			gettimeofday(&now_time, NULL);
		} while (get_time_diff(mark_time,now_time) < PERIOD_GET);
	}

	return 0;
}

int client_repeat_set_single() {
	// connect
	void* conn = storage_connect(SERVER_HOST,SERVER_PORT);
	// authenticate
	storage_auth(USERNAME,PASSWORD,conn);
	// set a data entry
	struct storage_record r1;
	strcpy(r1.value,VALUE);
	r1.metadata[0] = 0;
	storage_set(TABLE,KEY,&r1,conn);
	// repeatedly get a data entry
	while (1) {
		// get
		struct storage_record r1;
		strcpy(r1.value,VALUE);
		r1.metadata[0] = 0;
		struct timeval start_time, end_time;
		gettimeofday(&start_time, NULL);
		storage_set(TABLE,KEY,&r1,conn);
		gettimeofday(&end_time, NULL);
		printf("Set command done: used %ld microseconds\n", get_time_diff(start_time,end_time));
		// delay for a period
		struct timeval mark_time, now_time;
		gettimeofday(&mark_time, NULL);
		do {
			gettimeofday(&now_time, NULL);
		} while (get_time_diff(mark_time,now_time) < PERIOD_SET);
	}

	return 0;
}

int client_repeat_query() {
	// connect
	void* conn = storage_connect(SERVER_HOST,SERVER_PORT);
	// authenticate
	storage_auth(USERNAME,PASSWORD,conn);
	// repeatedly get a data entry
	while (1) {
		// query
		struct timeval start_time, end_time;
		gettimeofday(&start_time, NULL);
		char keys[MAX_RECORDS_PER_TABLE];
		int count = storage_query(TABLE,PREDICATES,keys,MAX_RECORDS_PER_TABLE,conn);
		gettimeofday(&end_time, NULL);
		printf("Query command done: used %ld microseconds\n", get_time_diff(start_time,end_time));
		// delay for a period
		struct timeval mark_time, now_time;
		gettimeofday(&mark_time, NULL);
		do {
			gettimeofday(&now_time, NULL);
		} while (get_time_diff(mark_time,now_time) < PERIOD_QUERY);
	}

	return 0;
}

int client_repeat_transaction() {
	// connect
	void* conn = storage_connect(SERVER_HOST,SERVER_PORT);
	// authenticate
	storage_auth(USERNAME,PASSWORD,conn);
	// set a data entry
	struct storage_record r1;
	strcpy(r1.value,VALUE);
	r1.metadata[0] = 0;
	storage_set(TABLE,KEY,&r1,conn);
	// record successful transactions and aborted transactions
	int success = 0, abort = 0;
	// repeatedly get a data entry
	while (1) {
		// get
		struct storage_record r2;
		storage_get(TABLE,KEY,&r2,conn);
		// delay for a period
		struct timeval mark_time, now_time;
		gettimeofday(&mark_time, NULL);
		do {
			gettimeofday(&now_time, NULL);
		} while (get_time_diff(mark_time,now_time) < PERIOD_TRANSACTION);
		// set
		int result = storage_set(TABLE,KEY,&r2,conn);
		// tally and display
		if (result == 0) {
			success++;
		} else {
			abort++;
		}
		printf("Total transactions: %d, successful transactions: %d, aborted transactions %d\n",
				success+abort,success,abort);
		// delay for a period
		gettimeofday(&mark_time, NULL);
		do {
			gettimeofday(&now_time, NULL);
		} while (get_time_diff(mark_time,now_time) < PERIOD_TRANSACTION);
		if (success + abort >= TOTAL_TRANSACTIONS) {
			break;
		}
	}

	return 0;
}




long get_time_diff(struct timeval before, struct timeval after) {
	return (after.tv_sec - before.tv_sec)*1000000L + after.tv_usec - before.tv_usec;
}

/**
 * @brief Start a client to interact with the storage server.
 *
 * If connect is successful, the client performs a storage_set/get() on
 * TABLE and KEY and outputs the results on stdout. Finally, it exists
 * after disconnecting from the server.
 */
int client_standard() {

	// Variables used throughout loop iterations
	int selection = -1;
	void* conn;

	// Initialize connection and authentication status checks to off
	is_connected = 0;
	is_authenticated = 0;

	// Starting a new log
	open_client_log();
	logger(client_log,"Starting new client session\n");

	// Start iteration
	while(selection != 8) {

		// Display menu
		printf("\n");
		printf("> ---------------------\n");
		printf("> 1) Connect           \n");
		printf("> 2) Authenticate      \n");
		printf("> 3) Get               \n");
		printf("> 4) Set (single entry)\n");
		printf("> 5) Set (from file)   \n");
		printf("> 6) Query             \n");
		printf("> 7) Disconnect        \n");
		printf("> 8) Exit              \n");
		printf("> ---------------------\n");

		// Accept selection input
		get_int_input("> Please enter your selection:",&selection,1,8);

		// Interpret selection
		switch (selection) {
		case 1:

		// Connect
		{
			// Dismiss if status check not met
			if (status_check(0,0) != 0) {
				continue;
			}
			// Accept input
			char server_host[MAX_HOST_LEN];
			int server_port = 0;
			if (get_string_input("> Please enter host name:",server_host,MAX_HOST_LEN) != 0
					|| get_int_input("> Please enter port:",&server_port,1000,9999) != 0) {
				continue;
			}

			// Issue storage_connect
			conn = storage_connect(server_host,server_port);
			if(!conn) {
				// Log error message
				sprintf(message,
						"Failed attempt to connect to server @ %s:%d. Error code: %d\n",
						server_host, server_port, errno);
				logger(client_log,message);
				// Feedback to client: failure
			    printf("Cannot connect to server @ %s:%d. Error code: %d\n",
			           server_host, server_port, errno);
			    continue;
			}
			// Feedback to client: success
			printf("> Successfully connected to server @ %s:%d\n",
						server_host, server_port);
			// Turn on check
			is_connected = 1;
			break;
		}

		case 2:

		// Authenticate
		{
			// Dismiss if status check not met
			if (status_check(1,0) != 0) {
				continue;
			}
			// Accept input
			char username[MAX_USERNAME_LEN], password[MAX_ENC_PASSWORD_LEN];
			if (get_string_input("> Please enter username:",username,MAX_USERNAME_LEN) != 0
					|| get_string_input("> Please enter password:",password,MAX_ENC_PASSWORD_LEN) != 0) {
				continue;
			}
			// Issue storage_auth
			int status = storage_auth(username,password,conn);
			if(status != 0) {
				// Log error message
				sprintf(message,"Failed attempt to authenticate with username '%s' and password '%s'. " \
						"Error code: %d\n", username, password, errno);
				logger(client_log,message);
				// Feedback to client: failure
				printf("> Cannot authenticate with username '%s' and password '%s' " \
						"Error code: %d\n", username, password, errno);
			    continue;
			}
			// Feedback to client: success
			printf("> Successfully authenticated with username '%s' and password '%s'\n",
					username, password);
			// Turn on check
			is_authenticated = 1;
			break;
		}

		case 3:

		// Get
		{
			// Dismiss if status check not met
			if (status_check(1,1) != 0) {
				continue;
			}
			// Accept input
			char table[MAX_TABLE_LEN], key[MAX_KEY_LEN];
			if (get_string_input("> Please enter table name:",table,MAX_TABLE_LEN) != 0
					|| get_string_input("> Please enter key:",key,MAX_KEY_LEN) != 0) {
				continue;
			}
			// Issue storage_get
			struct storage_record r;
			int status = storage_get(table, key, &r, conn);
			if(status != 0) {
				// Log error message
				sprintf(message,"Failed attempt to get data entry with table '%s' and key '%s'. " \
						"Error code: %d\n", table, key, errno);
				logger(client_log,message);
				// Feedback to client: failure
				printf("> Cannot get data entry with table '%s' and key '%s' " \
						"Error code: %d\n", table, key, errno);
			    continue;
			}
			// Feedback to client: success
			printf("> Successfully retrieved data: in table '%s' the key '%s' has a value '%s'.\n",
			         table, key, r.value);

			// Update record within transaction context
			char c, yn[256];
			while (1) {
				if (get_string_input("> Update this record? (Y/N):",yn,256) == 0) {
					c = yn[0];
					if (c == 'Y' || c == 'y' || c == 'N' || c == 'n') {
						break;
					}
				}
			}

			// User enters no
			if (c == 'N' || c == 'n') {
				continue;
			}

			// User enters yes
			char value[MAX_VALUE_LEN];
			if (get_string_input("> Please enter new value:",value,MAX_VALUE_LEN)) {
				continue;
			}
			strncpy(r.value, value, sizeof value);
			status = storage_set(table, key, &r, conn);
			if(status != 0) {
				// Log error message
				sprintf(message,"Failed attempt to update data entry with table '%s', key '%s', " \
						"and value '%s'. Error code: %d\n", table, key, value, errno);
				logger(client_log,message);
				// Feedback to client: failure
				printf("> Cannot update data entry with table '%s', key '%s', and value '%s'. " \
						"Error code: %d\n", table, key, value, errno);
				continue;
			}
			// Feedback to client: success
			printf("> Successfully updated data entry: in table '%s' key '%s' now has value '%s'\n",
					table, key, value);
			break;
		}

		case 4:

		// Set (single entry)
		{
			// Dismiss if status check not met
			if (status_check(1,1) != 0) {
				continue;
			}
			// Accept input
			char table[MAX_TABLE_LEN], key[MAX_KEY_LEN], value[MAX_VALUE_LEN];
			if (get_string_input("> Please enter table name:",table,MAX_TABLE_LEN) != 0
					|| get_string_input("> Please enter key:",key,MAX_KEY_LEN) != 0
					|| get_string_input("> Please enter value:",value,MAX_VALUE_LEN) != 0) {
				continue;
			}
			// Issue storage_set
			struct storage_record r;
			memset(r.metadata, 0, sizeof(r.metadata));
			strncpy(r.value, value, sizeof value);
			int status = storage_set(table, key, &r, conn);
			if(status != 0) {
				// Log error message
				sprintf(message,"Failed attempt to set data entry with table '%s', key '%s', " \
						"and value '%s'. Error code: %d\n", table, key, value, errno);
				logger(client_log,message);
				// Feedback to client: failure
				printf("> Cannot set data entry with table '%s', key '%s', and value '%s'. " \
						"Error code: %d\n", table, key, value, errno);
				continue;
			}
			// Feedback to client: success
			printf("> Successfully set data entry: in table '%s' key '%s' now has value '%s'\n",
					table, key, value);
			break;
		}

		case 5:

		// Set (from file)
		{
			// Dismiss if status check not met
			if (status_check(1,1) != 0) {
				continue;
			}
			// Accept input
			char table[MAX_TABLE_LEN], file_name[256],
				keys[MAX_RECORDS_PER_TABLE][MAX_KEY_LEN],
				values[MAX_RECORDS_PER_TABLE][MAX_VALUE_LEN];
			if (get_string_input("> Please enter table name:",table,MAX_TABLE_LEN) != 0
					|| get_string_input("> Please file name:",file_name,256) != 0) {
				continue;
			}
			// Initialize keys and values to null chars then fill in from file
			int k;
			for (k=0; k<MAX_RECORDS_PER_TABLE; k++) {
				keys[k][0] = '\0';
				values[k][0] = '\0';
			}

			if (extract_kv_from_file(file_name,keys,values) != 0) {
				continue;
			}

			// Issue storage_set with loop
			k = 0;
			struct timeval tvalBefore, tvalAfter;
			gettimeofday (&tvalBefore, NULL);
			while (keys[k][0] != '\0' && values[k][0] != '\0') {
				struct storage_record r;
				strncpy(r.value, values[k], sizeof values[k]);
				r.metadata[0] = 0;
				int status = storage_set(table, keys[k], &r, conn);
				if(status != 0) {
					// Log error message
					sprintf(message,"Failed attempt to set data entry with table '%s', key '%s', " \
							"and value '%s'. Error code: %d\n", table, keys[k], values[k], errno);
					logger(client_log,message);
					// Feedback to client: failure
					printf("> Cannot set data entry with table '%s', key '%s', and value '%s'. " \
							"Error code: %d\n", table, keys[k], values[k], errno);
					continue;
				}
				// Feedback to client: success
				//printf("> Successfully set data entry: in table '%s' key '%s' now has value '%s'\n",
				//		table, keys[k], values[k]);
				k++;
			}
			break;
		}

		case 6:
		{
			// Dismiss if status check not met
			if (status_check(1,1) != 0) {
				continue;
			}
			// Accept input
			char table[MAX_TABLE_LEN], predicates[MAX_VALUE_LEN];
			int max_keys = -1;
			if (get_string_input("> Please enter table name:",table,MAX_TABLE_LEN) != 0
					|| get_string_input("> Please enter predicates:",predicates,MAX_VALUE_LEN) != 0
					|| get_int_input("> Please enter max number of keys:",&max_keys,0,MAX_RECORDS_PER_TABLE) != 0) {
				continue;
			}
			// Issue storage_query
			char** keys = (char**)malloc(max_keys * sizeof(char*));
			int m;
			for (m=0; m<max_keys; m++) {
				keys[m] = (char*)malloc(MAX_KEY_LEN * sizeof(char));
			}
			int status = storage_query(table,predicates,keys,max_keys,conn);
			if(status < 0) {
				// Log error message
				sprintf(message,"Failed attempt to query with table '%s', predicates '%s', " \
							"Error code: %d\n", table, predicates, errno);
				logger(client_log,message);
				// Feedback to client: failure
				printf("> Cannot query with table '%s', predicates '%s'. " \
							"Error code: %d\n", table, predicates, errno);
				continue;
			}
			// Feedback to client: success
			char feedback[1024];
			sprintf(feedback,"> Successfully queried. In table '%s' " \
					"keys that match the predicates '%s' are: ",
					table,predicates);
			int k = 0;
			while (k < max_keys && keys[k][0] != '\0') {
				strcat(feedback,keys[k]);
				if (k < max_keys-1 && keys[k+1][0] != '\0') {
					strcat(feedback,", ");
				}
				k++;
			}
			printf("%s\n",feedback);
			break;
		}

		case 7:

		// Disconnect
		{
			// Dismiss if status check not met
			if (status_check(1,is_authenticated) != 0) {
				continue;
			}
			// Issue storage_disconnect
			int status = storage_disconnect(conn);
			if(status != 0) {
				// Log error message
				sprintf(message,"Failed attempt to disconnect from server. "\
						"Error code:%d\n",errno);
				logger(client_log,message);
				// Feedback to client: failure
				printf(message,"Cannot disconnect from server. "\
						"Error code:%d\n",errno);
				continue;
			}
			// Feed back to client: success
			printf("> Successfully disconnected from server\n");
			// Turn off checks
			is_connected = 0;
			is_authenticated = 0;
			break;
		}

		case 8:

		// Exit
		{
			printf("> Bye\n");
			// Disconnect if still connected
			if (is_connected) {
				storage_disconnect(conn);
			}
			// Close log file
			logger(client_log,"Ending this client session\n");
			close_client_log();
			break;
		}

		default:

		// Invalid selection (do nothing)
		{
			break;
		}

		}
	}

	// Exit
	return 0;
}

/**
 * Helper function to accept textual input from user
 */
int get_string_input(char* prompt, char dest[], int length) {
	printf("%s ",prompt);
	fgets(dest,length,stdin);
	dest[strcspn(dest,"\n")] = '\0';
	/*
	if (dest[0] == '\0') {
		printf("> Invalid input\n");
		return 1;
	}
	*/
	return 0;
}

/**
 * Helper function to accept numerical input from user
 */
int get_int_input(char* prompt, int* dest, int low_bound, int up_bound) {
	printf("%s ",prompt);
	char buffer[MAX_CMD_LEN];
	fgets(buffer,MAX_CMD_LEN,stdin);
	buffer[strcspn(buffer,"\n")] = '\0';
	*dest = atoi(buffer);
	if (*dest < low_bound || *dest > up_bound) {
		printf("> Invalid input\n");
		return 1;
	}
	return 0;
}

/**
 * Helper function to check whether client's current connection and authentication
 * status meets specified requirements
 */
int status_check(int shoud_be_connected, int should_be_authenticated) {
	if (shoud_be_connected && !is_connected) {
		printf("> You are not connected yet\n");
		return 1;
	} else if (!shoud_be_connected && is_connected) {
		printf("> You are already connected\n");
		return 1;
	} else if (should_be_authenticated && !is_authenticated) {
		printf("> You are not authenticated yet\n");
		return 1;
	} else if (!should_be_authenticated && is_authenticated) {
		printf("> You are already authenticated\n");
		return 1;
	}
	return 0;
}



