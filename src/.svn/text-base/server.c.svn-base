/**
 * @file
 * @brief This file implements the storage server.
 *
 * The storage server should be named "server" and should take a single
 * command line argument that refers to the configuration file.
 * 
 * The storage server should be able to communicate with the client
 * library functions declared in storage.h and implemented in storage.c.
 */

// Test comment by David

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#include "utils.h"
#include <time.h>
#include "database.h"
#include "parse_utils.h"
#include <pthread.h>

#define MAX_LISTENQUEUELEN 20	///< The maximum number of queued connections.

struct config_params params;
long accumulated_set_time;


// thread subroutine
void* wait_for_commands();
// thread argument structure
struct thread_data {
	int clientsock;
	struct sockaddr_in clientaddr;
};
// thread count
int thread_count;

// commands
void command_set(char* cmd,
		char table_name[MAX_ARG_VAL_LEN],
		char key[MAX_ARG_VAL_LEN],
		char value[MAX_ARG_VAL_LEN],
		char metadata[MAX_ARG_VAL_LEN]);
void command_get(char* cmd,
		char table_name[MAX_ARG_VAL_LEN],
		char key[MAX_ARG_VAL_LEN]);
void command_query(char* cmd,
		char table_name[MAX_ARG_VAL_LEN],
		char max[MAX_ARG_VAL_LEN],
		char predicates[MAX_ARG_VAL_LEN]);
// helpers
int table_check(char* table);
int key_check(char* key);
int value_check(char* value);

/**
 * @brief Start the storage server.
 *
 * This is the main entry point for the storage server.  It reads the
 * configuration file, starts listening on a port, and proccesses
 * commands from clients.
 */
int main(int argc, char *argv[])
{
	// Starting a new log
	open_server_log();

	// Set time to zero
	accumulated_set_time = 0;

	// Process command line arguments.
	// This program expects exactly one argument: the config file name.
	assert(argc > 0);
	if (argc != 2) {
		printf("Usage %s <config_file>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	char *config_file = argv[1];

	// Read the config file.
	int status = read_config(config_file, &params);
	if (status != 0) {
		printf("Error processing config file.\n");
		exit(EXIT_FAILURE);
	}

	// Log: server host name and port
	sprintf(message,"Server on %s:%d\n", params.server_host, params.server_port);
	logger(server_log,message);

	// Log: concurrency settings
	sprintf(message,"Concurrency parameter: %d\n",params.concurrency);
	logger(server_log,message);

	// Database: initialize table schema
	status = init_tables(params.tables);
	if (status != 0) {
		exit(EXIT_FAILURE);
	}
	// Log: table schema
	sprintf(message,"Database has %d tables:\n",table_count);
	logger(server_log,message);
	int k,m;
	for (k=0; k<table_count; k++) {
		sprintf(message,"  -Table '%s' : { ",tables[k]->name);
		for (m=0; m<tables[k]->col_count; m++) {
			char col_name[30],col_type[30];
			strcpy(col_name,tables[k]->columns[m]->name);
			if (tables[k]->columns[m]->type == INT) {
				strcpy(col_type,"int");
			} else if (tables[k]->columns[m]->type == CHAR) {
				sprintf(col_type,"char[%d]",tables[k]->columns[m]->str_len);
			}
			char buff[60];
			sprintf(buff,"%s(%s)",col_name,col_type);
			strcat(message,buff);
			if (m < tables[k]->col_count-1) {
				strcat(message,", ");
			}
		};
		strcat(message," }\n");
		logger(server_log,message);
	}


	// Create a socket.
	int listensock = socket(PF_INET, SOCK_STREAM, 0);
	if (listensock < 0) {
		printf("Error creating socket.\n");
		exit(EXIT_FAILURE);
	}

	// Allow listening port to be reused if defunct.
	int yes = 1;
	status = setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
	if (status != 0) {
		printf("Error configuring socket.\n");
		exit(EXIT_FAILURE);
	}

	// Bind it to the listening port.
	struct sockaddr_in listenaddr;
	memset(&listenaddr, 0, sizeof listenaddr);
	listenaddr.sin_family = AF_INET;
	listenaddr.sin_port = htons(params.server_port);
	inet_pton(AF_INET, params.server_host, &(listenaddr.sin_addr)); // bind to local IP address
	status = bind(listensock, (struct sockaddr*) &listenaddr, sizeof listenaddr);
	if (status != 0) {
		printf("Error binding socket.\n");
		exit(EXIT_FAILURE);
	}

	// Listen for connections.
	status = listen(listensock, MAX_LISTENQUEUELEN);
	if (status != 0) {
		printf("Error listening on socket.\n");
		exit(EXIT_FAILURE);
	}

	// Listen loop.
	int wait_for_connections = 1;
	thread_count = 0;
	while (wait_for_connections) {
		// Wait for a connection.
		struct sockaddr_in clientaddr;
		socklen_t clientaddrlen = sizeof clientaddr;
		int clientsock = accept(listensock, (struct sockaddr*)&clientaddr, &clientaddrlen);
		if (params.concurrency == 0 && thread_count > 0) {
			continue;
		}

		// Create new thread
		pthread_t thread;
		int result;
		struct thread_data* data =
				(struct thread_data*)malloc(sizeof(struct thread_data));
		data->clientaddr = clientaddr;
		data->clientsock = clientsock;
		result = pthread_create(&thread,NULL,wait_for_commands,(void*)data);
		if (result != 0) {
			sprintf(message,"Error: failed to create new thread. Returned %d\n",
					result);
			logger(server_log,message);
		}
	}

	// Stop listening for connections.
	close(listensock);

	// Close log file
	close_server_log();

	pthread_exit(NULL);
	return EXIT_SUCCESS;
}



void* wait_for_commands(void* thread_arg) {
	// Log: new thread created
	thread_count++;
	sprintf(message,"Created new thread. There are currently %d " \
			"active thread(s)\n", thread_count);
	logger(server_log,message);

	struct thread_data* data = (struct thread_data*) thread_arg;
	int clientsock = data->clientsock;
	struct sockaddr_in clientaddr= data->clientaddr;
	if (clientsock < 0) {
		printf("Error accepting a connection.\n");
		exit(EXIT_FAILURE);
	}

	sprintf(message,"Got a connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
	logger(server_log,message);

	// Get commands from client.
	int wait_for_commands = 1;
	do {
		// Read a line from the client.
		char cmd[MAX_CMD_LEN];
		int status = recvline(clientsock, cmd, MAX_CMD_LEN);
		if (status != 0) {
			// Either an error occurred or the client closed the connection.
			wait_for_commands = 0;
		} else {
			// Handle the command from the client.
			int status = handle_command(clientsock, cmd);
			if (status != 0)
				wait_for_commands = 0; // Oops.  An error occured.
		}
	} while (wait_for_commands);

	// Close the connection with the client.
	close(clientsock);

	sprintf(message,"Closed connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
	logger(server_log,message);

	// Log: thread terminated
	thread_count--;
	sprintf(message,"Terminating a thread. There are currently %d " \
			"active thread(s)\n", thread_count);
	logger(server_log,message);

	pthread_exit(NULL);
}

/**
 * @brief Process a command from the client.
 *
 * @param sock The socket connected to the client.
 * @param cmd The command received from the client.
 * @return Returns 0 on success, -1 otherwise.
 */

int handle_command(int sock, char *cmd)
{

	sprintf(message,"Processing command '%s'\n", cmd);
	logger(server_log,message);

	// Parse command
	struct protocol_arg_pair* args[MAX_ARG_NUM];
	char cmd_buff[MAX_CMD_LEN];
	strncpy(cmd_buff,cmd,sizeof(cmd));
	extract_arg_from_line(args,cmd);

	// Extract argument value
	char action[MAX_ARG_VAL_LEN];
	get_arg_val(args,"action",action);

	if (strcmp(action,"authenticate") == 0) {
		// authentication
		char username[MAX_ARG_VAL_LEN], password[MAX_ARG_VAL_LEN];
		get_arg_val(args,"username",username);
		get_arg_val(args,"password",password);
		if (strcmp(params.username,username) == 0
				&& strcmp(params.password,password) == 0) {
			cmd = "status=0!";
		} else {
			cmd = "status=-1!";
		}
	} else if (strcmp(action,"get") == 0) {
		// get
		char table[MAX_ARG_VAL_LEN], key[MAX_ARG_VAL_LEN];
		get_arg_val(args,"table",table);
		get_arg_val(args,"key",key);
		command_get(cmd,table,key);
	} else if (strcmp(action,"set") == 0) {
		// set
		char table[MAX_ARG_VAL_LEN], key[MAX_ARG_VAL_LEN], value[MAX_ARG_VAL_LEN], metadata[MAX_ARG_VAL_LEN];
		get_arg_val(args,"table",table);
		get_arg_val(args,"key",key);
		get_arg_val(args,"value",value);
		get_arg_val(args,"metadata",metadata);
		command_set(cmd,table,key,value,metadata);
	} else if (strcmp(action,"query") == 0) {
		// query
		char table[MAX_ARG_VAL_LEN], max[MAX_ARG_VAL_LEN], predicates[MAX_ARG_VAL_LEN];
		get_arg_val(args,"table",table);
		get_arg_val(args,"max",max);
		get_arg_val(args,"predicates",predicates);
		command_query(cmd,table,max,predicates);
	}

	sprintf(message,"Response to client: '%s'\n",cmd);
	logger(server_log,message);

	// Send response
	sendall(sock, cmd, strlen(cmd));
	sendall(sock, "\n", 1);

	return 0;
}



void command_set(char* cmd,
		char table_name[MAX_ARG_VAL_LEN],
		char key[MAX_ARG_VAL_LEN],
		char value[MAX_ARG_VAL_LEN],
		char metadata[MAX_ARG_VAL_LEN]) {
	if (table_check(table_name) != 0 || key_check(key) !=0) {
		strcpy(cmd,"status=-1#error=1!");
		return;
	} else {
		// find table with given table name
		struct data_table* table_p = find_table(table_name);
		if (table_p == 0) {
			sprintf(message,"Error: unknown table name '%s'\n",table_name);
			logger(server_log,message);
			strcpy(cmd,"status=-1#error=5!");
			return;
		} else if (strcmp(value,"{((NULL))}") == 0) {
			int result = delete_entry(table_p,key);
			if (result != 0) {
				strcpy(cmd,"status=-1#error=6!");
			} else {
				strcpy(cmd,"status=0!");
			}
			return;
		} else {
			// get rid of the leading '{' and trailing '}'
			char value_buff[256];
			strncpy(value_buff,&value[1],strlen(value)-2);
			value_buff[strlen(value)-2] = '\0';
			// value_arr that will be used to call set_entry function
			char value_arr[MAX_COLUMNS_PER_TABLE][MAX_VALUE_LEN];
			// traverse through value_buff
			char* p = &value_buff[0];
			int col_index = 0;
			while (1) {
				// find the next comma-separated chunk
				char temp_t[256];
				int k = get_next_text_chunk(p,',',temp_t);
				if (p-&value_buff[0] >= strlen(value_buff)) {
					if (col_index != table_p->col_count) {
						logger(server_log,"Error: too few arguments in value\n");
						strcpy(cmd,"status=-1#error=1");
						return;
					}
					break;
				}
				if (col_index >= table_p->col_count) {
					// too many arguments
					logger(server_log,"Error: too many arguments in value\n");
					strcpy(cmd,"status=-1#error=1");
					return;
				}
				// take out trailing and leading white-space
				char temp[256];
				delete_leading_trailing_spaces(temp_t,temp);
				// find the next space-separated chunk
				char col_name[256];
				get_next_text_chunk(temp,' ',col_name);
				if (strcmp(table_p->columns[col_index]->name,
						col_name) != 0) {
					// unknown column name
					sprintf(message,"Error: unknown column name '%s' at index '%d'\n",
							col_name,col_index);
					logger(server_log,message);
					strcpy(cmd,"status=-1#error=1");
					return;
				}
				// fill value_arr
				strcpy(value_arr[col_index],&temp[strlen(col_name)+1]);
				// check value type limitations
				if (table_p->columns[col_index]->type == INT
						&& check_numeric(value_arr[col_index]) != 0) {
					sprintf(message,"Error: value for column '%s' is not numeric\n",
							table_p->columns[col_index]->name);
					logger(server_log,message);
					strcpy(cmd,"status=-1#error=1");
					return;
				} else if (table_p->columns[col_index]->type == CHAR
						&& strlen(value_arr[col_index]) > table_p->columns[col_index]->str_len) {
					sprintf(message,"Error: length of value for column '%s' is too long\n",
							table_p->columns[col_index]->name);
					logger(server_log,message);
					strcpy(cmd,"status=-1#error=1");
					return;
				}
				// increment col_index
				col_index++;
				// go to next chunk
				p += (k+1);
			}
			// at this point, our value_arr is filled
			int result = set_entry(table_p,key,value_arr,atoi(metadata));
			if (result != 0) {
				strcpy(cmd,"status=-1#error=8!");
				return;
			}
			strcpy(cmd,"status=0!");
		}
	}
}

void command_get(char* cmd,
		char table_name[MAX_ARG_VAL_LEN],
		char key[MAX_ARG_VAL_LEN]) {
	if (table_check(table_name) != 0 || key_check(key) !=0) {
		strcpy(cmd,"status=-1#error=1!");
		return;
	} else {
		// find table with given table name
		struct data_table* table_p = find_table(table_name);
		if (table_p == 0) {
			sprintf(message,"Error: unknown table name '%s'\n",table_name);
			logger(server_log,message);
			strcpy(cmd,"status=-1#error=5!");
			return;
		} else {
			struct data_entry* entry = find_entry(table_p,key);
			if (entry == 0) {
				sprintf(message,"Error: key '%s' not found in table '%s'\n",
						key,table_name);
				logger(server_log,message);
				strcpy(cmd,"status=-1#error=6!");
				return;
			}
			char value_buff[MAX_VALUE_LEN];
			strcpy(value_buff,"");
			int col_index = 0;
			for (col_index=0; col_index<table_p->col_count; col_index++) {
				char temp[MAX_VALUE_LEN];
				sprintf(temp,"%s %s",
						table_p->columns[col_index]->name,
						entry->value[col_index]);
				strcat(value_buff,temp);
				if (col_index < table_p->col_count -1) {
					strcat(value_buff,", ");
				}
			}
			sprintf(cmd,"status=0#value=%s#metadata=%d!",value_buff,entry->metadata);
		}
	}
}

void command_query(char* cmd,
		char table_name[MAX_ARG_VAL_LEN],
		char max[MAX_ARG_VAL_LEN],
		char predicates[MAX_ARG_VAL_LEN]) {
	flush_query_params();
	if (table_check(table_name) != 0 || check_numeric(max) !=0) {
		strcpy(cmd,"status=-1#error=1!");
	} else {
		struct data_table* table_p = find_table(table_name);
		int max_keys = atoi(max);
		int keys_acquired = 0;
		char keys[MAX_RECORDS_PER_TABLE][MAX_KEY_LEN];
		if (table_p == 0) {
			sprintf(message,"Error: unknown table name '%s'\n",table_name);
			logger(server_log,message);
			strcpy(cmd,"status=-1#error=5!");
			return;
		} else {
			// get rid of the leading '{' and trailing '}'
			char pred_buff[256];
			strncpy(pred_buff,&predicates[1],strlen(predicates)-2);
			pred_buff[strlen(predicates)-2] = '\0';
			char *p = &pred_buff[0];
			while (1) {
				// extract each chunk of conditions
				char temp_t[256];
				int k = get_next_text_chunk(p,',',temp_t);
				if (k==0) {
					break;
				}
				// take out trailing and leading white-space
				char temp[256];
				delete_leading_trailing_spaces(temp_t,temp);
				// extract parts
				char col_name[MAX_COLNAME_LEN];
				char operand[2];
				char comp_val[MAX_VALUE_LEN];
				//int items = sscanf(temp,"%s %s %s %s",col_name,operand,comp_val,excess);
				int parse = parse_predicates(temp,col_name,operand,comp_val);
				if (parse != 0) {
					sprintf(message,"Error: predicates condition '%s' "\
							"has bad format\n",temp);
					logger(server_log,message);
					strcpy(cmd,"status=-1#error=1!");
					return;
				}
				int result = set_query_params(table_p,col_name,operand,comp_val);
				if (result != 0) {
					sprintf(message,"Error: predicates condition '%s' "\
							"has bad content\n",temp);
					logger(server_log,message);
					strcpy(cmd,"status=-1#error=1!");
					return;
				}
				p += (k+1);
			}

			query(table_p,keys,max_keys,&keys_acquired);
		}

		int k = 0;
		char keys_buff[MAX_VALUE_LEN];
		strcpy(keys_buff,"");
		for (k=0; k<keys_acquired; k++) {
			strcat(keys_buff,keys[k]);
			if (k < keys_acquired -1) {
				strcat(keys_buff,",");
			}
		}
		sprintf(cmd,"status=0#num=%d#keys={%s}!",keys_acquired,keys_buff);
		return;
	}
}







/**
 * Helper function to check whether an input for table name has acceptable format
 */
int table_check(char* table) {
	char* allowed_characters = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char* p = table;
	while (*p != '\0') {
		if (strchr(allowed_characters,*p) == NULL) {
			sprintf(message,"Input '%s' for table has unacceptable character(s)\n",table);
			logger(server_log,message);
			return 1;
		}
		p++;
	}
	return 0;
}

/**
 * Helper function to check whether an input for key name has acceptable format
 */
int key_check(char* key) {
	char* allowed_characters = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char* p = key;
	while (*p != '\0') {
		if (strchr(allowed_characters,*p) == NULL) {
			sprintf(message,"Input '%s' for key has unacceptable character(s)\n",key);
			logger(server_log,message);
			return 1;
		}
		p++;
	}
	return 0;
}

/**
 * Helper function to check whether an input for value has acceptable format
 */
int value_check(char* value) {
	char* allowed_characters = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	char* p = value;
	while (*p != '\0') {
		if (strchr(allowed_characters,*p) == NULL) {
			sprintf(message,"Input '%s' for value has unacceptable character(s)\n",value);
			logger(server_log,message);
			return 1;
		}
		p++;
	}
	return 0;
}




