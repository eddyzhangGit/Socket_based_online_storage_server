/**
 * @file
 * @brief This file implements various utility functions that are
 * can be used by the storage server and client library. 
 */

#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "utils.h"
#include "parse_utils.h"

int sendall(const int sock, const char *buf, const size_t len)
{
	size_t tosend = len;
	while (tosend > 0) {
		ssize_t bytes = send(sock, buf, tosend, 0);
		if (bytes <= 0) 
			break; // send() was not successful, so stop.
		tosend -= (size_t) bytes;
		buf += bytes;
	};

	return tosend == 0 ? 0 : -1;
}

/**
 * In order to avoid reading more than a line from the stream,
 * this function only reads one byte at a time.  This is very
 * inefficient, and you are free to optimize it or implement your
 * own function.
 */
int recvline(const int sock, char *buf, const size_t buflen)
{
	int status = 0; // Return status.
	size_t bufleft = buflen;

	while (bufleft > 1) {
		// Read one byte from scoket.
		ssize_t bytes = recv(sock, buf, 1, 0);
		if (bytes <= 0) {
			// recv() was not successful, so stop.
			status = -1;
			break;
		} else if (*buf == '\n') {
			// Found end of line, so stop.
			*buf = 0; // Replace end of line with a null terminator.
			status = 0;
			break;
		} else {
			// Keep going.
			bufleft -= 1;
			buf += 1;
		}
	}
	*buf = 0; // add null terminator in case it's not already there.

	return status;
}


/**
 * @brief Parse and process a line in the config file.
 */
int process_config_line(char *line, struct config_params *params)
{
	// Ignore comments.
	if (line[0] == CONFIG_COMMENT_CHAR)
		return 0;

	// Extract config parameter name and value.
	char name[MAX_CONFIG_LINE_LEN];
	char value[MAX_CONFIG_LINE_LEN];
	char columns[MAX_CONFIG_LINE_LEN];

	int items = sscanf(line, "%s %s", name, value);
	// Line wasn't as expected.
	if (items != 2) {
		sprintf(message,"Error: bad number of items in config file line: %s",line);
		return -1;
	}

	// Process this line.
	if (strcmp(name, "server_host") == 0) {
		if (*(params->server_host) != '\0') {
			logger(server_log,"Error: multiple server host entries in config file\n");
			return -1;
		}
		strncpy(params->server_host, value, sizeof params->server_host);
	} else if (strcmp(name, "server_port") == 0) {
		if (params->server_port != -1) {
			logger(server_log,"Config file error: multiple server port entries\n");
			return -1;
		}
		params->server_port = atoi(value);
	} else if (strcmp(name, "username") == 0) {
		if (*(params->username) != '\0') {
			logger(server_log,"Config file error: multiple username entries\n");
			return -1;
		}
		strncpy(params->username, value, sizeof params->username);
	} else if (strcmp(name, "password") == 0) {
		if (*(params->password) != '\0') {
			logger(server_log,"Config file error: multiple password entries\n");
			return -1;
		}
		strncpy(params->password, value, sizeof params->password);
	} else if (strcmp(name, "concurrency") == 0) {
		if (params->concurrency != -1) {
			logger(server_log,"Config file error: multiple concurrency entries\n");
			return -1;
		}
		params->concurrency = atoi(value);
	} else if (strcmp(name, "table") == 0) {
		int k=0;
		while (params->tables[k]!=0){
			if (strcmp(params->tables[k]->name,value) == 0) {
				sprintf(message,"Config file error: multiple "\
						"table entries '%s'\n",value);
				logger(server_log,message);
				return -1;
			}
			k++;
		}
		params->tables[k] = (struct table*)malloc(sizeof(struct table));
		strncpy(params->tables[k]->name,value,sizeof params->tables[k]->name);
		params->tables[k]->col_count = 0;
		// fill in column information
		if (strlen(name)+strlen(value)+2 >= strlen(line)) {
			// no column information provided
			sprintf(message,"Error: table '%s' is missing column information\n",name);
			logger(server_log,message);
			return -1;
		}
		char cols[MAX_CONFIG_LINE_LEN];
		strcpy(cols,line+strlen(name)+strlen(value)+2);
		int m=0;
		char* p = strtok(cols," ,\n");
		while (p != NULL) {
			params->tables[k]->columns[m] = (struct column*)malloc(sizeof(struct column));
			char* dilim = strchr(p,':');
			strncpy(params->tables[k]->columns[m]->name,p,dilim-p);
			params->tables[k]->columns[m]->name[dilim-p] = '\0';
			strcpy(params->tables[k]->columns[m]->type,dilim+1);
			p = strtok(NULL," ,\n");
			m++;
		}
		params->tables[k]->col_count = m;
	}
	// else if (strcmp(name, "data_directory") == 0) {
	//	strncpy(params->data_directory, value, sizeof params->data_directory);
	//} 
	else {
		// Ignore unknown config parameters.
	}
	return 0;
}


int read_config(const char *config_file, struct config_params *params)
{
	int error_occurred = 0;

	// Open file for reading.
	FILE *file = fopen(config_file, "r");
	if (file == NULL)
		error_occurred = 1;

	// Initialize params memebers' initial value
	*(params->server_host) = '\0';
	params->server_port = -1;
	*(params->username) = '\0';
	*(params->password) = '\0';
	params->concurrency = -1;
	int k;
	for (k=0; k<MAX_TABLES; k++) {
		params->tables[k] = 0;
	}

	// Process the config file.
	while (!error_occurred && !feof(file)) {
		// Read a line from the file.
		char line[MAX_CONFIG_LINE_LEN];
		char *l = fgets(line, sizeof line, file);

		// Process the line.
		if (l == line) {
			if (process_config_line(line, params) != 0)
				error_occurred = 1;
		}
		else if (!feof(file))
			error_occurred = 1;
	}
	return error_occurred ? -1 : 0;
}

void logger(FILE *file, char *message)
{
	// Create time stamp
	char time_stamp[sizeof(TIME_STAMP_TEMPLATE)+2];
	strftime(time_stamp,
			sizeof(time_stamp),
			TIME_STAMP_TEMPLATE,
			get_time_info());

	// log the message according to LOGGING parameter
	if (LOGGING == 0) {
		// logging disabled (do nothing)
	} else if (LOGGING == 1) {
		// logging to stdout
		printf("%s %s",time_stamp,message);
	} else if (LOGGING == 2 && file != NULL) {
		// logging to log file
		fprintf(file,"%s %s",time_stamp,message);
		fflush(file);
	}

}

char *generate_encrypted_password(const char *passwd, const char *salt)
{
	if(salt != NULL)
		return crypt(passwd, salt);
	else
		return crypt(passwd, DEFAULT_CRYPT_SALT);
}



/**
 * Additional function implementations to facilitate logging
 */
void open_client_log() {
	if (client_log == NULL && LOGGING == 2) {
		char log_file_name[MAX_LOG_FILE_NAME_LEN];
		strftime(log_file_name,
				sizeof(log_file_name),
				CLIENT_LOG_FILE_NAME,
				get_time_info());
		client_log = fopen(log_file_name,"w");
	}
}
void close_client_log() {
	if (client_log != NULL) {
		fclose(client_log);
	}
}
void open_server_log() {
	if (server_log == NULL && LOGGING == 2) {
		char log_file_name[MAX_LOG_FILE_NAME_LEN];
		strftime(log_file_name,
				sizeof(log_file_name),
				SERVER_LOG_FILE_NAME,
				get_time_info());
		server_log = fopen(log_file_name,"w");
	}
}
void close_server_log() {
	if (server_log != NULL) {
			fclose(server_log);
	}
}

struct tm* get_time_info() {
	time_t rawtime;
	struct tm* time_info;
	time(&rawtime);
	time_info = localtime(&rawtime);
	return time_info;
}


/**
 * Communication protocol helper
 */
void extract_arg_from_line(
		struct protocol_arg_pair* args[MAX_ARG_NUM],
		char line[MAX_CMD_LEN]) {
	//printf("%s\n",line);
	int k;
	for (k=0; k<MAX_ARG_NUM; k++) {
		args[k] = 0;
	}
	//printf("0\n");
	char* head = line;
	char* tail = line;
	k = 0;
	while (*head != TERMINATE_CHAR) {
		//printf("1 !!%d\n",k);
		struct protocol_arg_pair* temp_args =
				(struct protocol_arg_pair*)malloc(sizeof(protocol_arg_pair));
		while (*head != '=') {
			head++;
		}
		//printf("2 !!%d\n",k);
		strncpy(temp_args->arg_name,tail,head-tail);
		temp_args->arg_name[head-tail] = '\0';
		tail = head + 1;

		while (*head != '#' && *head != TERMINATE_CHAR) {
			head++;
		}
		//printf("3 !!%d\n",k);
		strncpy(temp_args->arg_val,tail,head-tail);
		temp_args->arg_val[head-tail] = '\0';
		tail = head + 1;
		args[k] = temp_args;
		//printf("4 !!%d\n",k);
		k++;
	}
}

/**
 * Get argument value
 */
int	get_arg_val(
		struct protocol_arg_pair* args[MAX_ARG_NUM],
		char* name,
		char value[MAX_ARG_VAL_LEN]) {
	int k = 0;
	while (args[k] != 0) {
		if (strcmp(args[k]->arg_name,name) == 0) {
			strncpy(value,args[k]->arg_val,sizeof(args[k]->arg_val));
			return 0;
		}
		k++;
	}
	return 1;
}



/**
 * Mass SET with text file
 */
int extract_kv_from_file(char* file_name,char keys[MAX_RECORDS_PER_TABLE][MAX_KEY_LEN],
			 char values[MAX_RECORDS_PER_TABLE][MAX_VALUE_LEN]) {


	    FILE *infile;
	    char line_buffer[BUFSIZ]; /* BUFSIZ is defined if you include stdio.h */
	    int line_number;
	    infile = fopen(file_name, "r");
	    if (!infile) {
	        printf("Couldn't open file %s for reading.\n", file_name);
	        return 0;
	    }
	    printf("Opened file %s for reading.\n", file_name);

	    line_number = 0;
	    while (fgets(line_buffer, sizeof(line_buffer), infile)) {
	        ++line_number;
	    int i=0;
	    int j=0;
            char key[200];
            while(line_buffer[i]!=',')
           {
             key[i]=line_buffer[i];
             i++;
           }

           key[i]='\0';
           char value[200];
	   i++;
           while(line_buffer[i]!='\n')
           {
             value[j]=line_buffer[i];
             i++;
             j++;
           }


           value[j]='\0';

	   // printf("%s\n",value);
	   // printf("%s\n",key);
            strcpy(keys[line_number-1],key);
	    strcpy(values[line_number-1],value);


	    }
	    return 0;


}




