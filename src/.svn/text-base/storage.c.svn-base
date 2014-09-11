/**
 * @file
 * @brief This file contains the implementation of the storage server
 * interface as specified in storage.h.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "storage.h"
#include "utils.h"
#include "parse_utils.h"
#include <errno.h>
#include <time.h>
/**
 * @brief This is just a minimal stub implementation.  You should modify it 
 * according to your design.
 */
void* storage_connect(const char *hostname, const int port)
{
	// Check parameters
	if (hostname == NULL
			|| strlen(hostname) == 0
			|| port == NULL
			|| port < 0) {
		errno = 1;
		return NULL;
	}
	// Logger call
	sprintf(message,
			"Received a CONNECT command with hostname:'%s' port:'%d'\n",
			hostname,
			port);
	logger(client_log,message);

	// Create a socket.
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
			return NULL;

	// Get info about the server.
	struct addrinfo serveraddr, *res;
	memset(&serveraddr, 0, sizeof serveraddr);
	serveraddr.ai_family = AF_UNSPEC;
	serveraddr.ai_socktype = SOCK_STREAM;
	char portstr[MAX_PORT_LEN];
	snprintf(portstr, sizeof portstr, "%d", port);
	int status = getaddrinfo(hostname, portstr, &serveraddr, &res);
	if (status != 0) {
		errno = 1;
		return NULL;
	}

	// Connect to the server.
	status = connect(sock, res->ai_addr, res->ai_addrlen);
	if (status != 0) {
		errno = 2;
		return NULL;
	}

	return (void*) sock;
}


/**
 * @brief This is just a minimal stub implementation.  You should modify it 
 * according to your design.
 */
int storage_auth(const char *username, const char *passwd, void *conn)
{
	// Check parameters
	if (username == NULL
			|| passwd == NULL
			|| strlen(username) == 0
			|| strlen(passwd) == 0
			|| conn == NULL) {
		errno = 1;
		return -1;
	}
	// Logger call
	sprintf(message,
			"Received an AUTHENTICATE command with username:'%s' password:'%s'\n",
			username,
			passwd);
	logger(client_log,message);

	// Connection is really just a socket file descriptor.
	int sock = (int)conn;

	// Send some data.
	char buf[MAX_CMD_LEN];
	memset(buf, 0, sizeof buf);
	char *encrypted_passwd = generate_encrypted_password(passwd, NULL);
	snprintf(buf, sizeof buf,
			"action=authenticate#username=%s#password=%s!\n",
			username, encrypted_passwd);

	if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0) {
		// Log server's response
		sprintf(message,
				"Server's response: '%s'\n",
				buf);
		logger(client_log,message);
		// Parse response
		struct protocol_arg_pair* args[MAX_ARG_NUM];
		extract_arg_from_line(args,buf);
		// Get status
		char status[MAX_ARG_VAL_LEN];
		get_arg_val(args,"status",status);
		if (strcmp(status,"0") == 0) {
			return 0;
		} else {
			errno = 4;
			return -1;
		}
	}
	errno = 7;
	return -1;
}

/**
 * @brief This is just a minimal stub implementation.  You should modify it 
 * according to your design.
 */
int storage_get(const char *table, const char *key, struct storage_record *record, void *conn)
{
	// Check parameters
	if (table == NULL
			|| key == NULL
			|| record == NULL
			|| conn == NULL
			|| strlen(table) == 0
			|| strlen(key) == 0) {
		errno = 1;
		return -1;
	}
	// Logger call
	sprintf(message,
			"Received a GET command with table:'%s' key:'%s'\n",
			table,
			key);
	logger(client_log,message);

	// Connection is really just a socket file descriptor.
	int sock = (int)conn;

	// Send some data.
	char buf[MAX_CMD_LEN];
	memset(buf, 0, sizeof buf);
	snprintf(buf, sizeof buf,
			"action=get#table=%s#key=%s!\n",
			table, key);

	if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0) {
		// Log server's response
		sprintf(message,
				"Server's response: '%s'\n",
				buf);
		logger(client_log,message);
		// Parse response
		struct protocol_arg_pair* args[MAX_ARG_NUM];
		extract_arg_from_line(args,buf);
		// Get status
		char status[MAX_ARG_VAL_LEN];
		get_arg_val(args,"status",status);
		if (strcmp(status,"0") == 0) {
			char value[MAX_ARG_VAL_LEN],metadata[MAX_ARG_VAL_LEN];
			get_arg_val(args,"value",value);
			get_arg_val(args,"metadata",metadata);
			strcpy(record->value,value);
			record->metadata[0] = atoi(metadata);
			return 0;
		} else {
			char error[MAX_ARG_VAL_LEN];
			get_arg_val(args,"error",error);
			errno = error[0] - '0';
			return -1;
		}
	}
	errno = 7;
	return -1;
}


/**
 * @brief This is just a minimal stub implementation.  You should modify it 
 * according to your design.
 */
int storage_set(const char *table, const char *key, struct storage_record *record, void *conn)
{
	// Check parameters
	if (table == NULL
			|| key == NULL
			|| conn == NULL
			|| strlen(table) == 0
			|| strlen(key) == 0) {
			errno = 1;
			return -1;
	}

	char value[MAX_VALUE_LEN];
	int md;
	if (record == NULL || strlen(record->value)==0) {
		strcpy(value,"((NULL))");
		md = 0;
	} else {
		strcpy(value,record->value);
		md = record->metadata[0];
	}

	// Logger call
	sprintf(message,
			"Received a SET command with table:'%s' key:'%s' value:'%s'\n",
			table,
			key,
			value);
	logger(client_log,message);

	// Connection is really just a socket file descriptor.
	int sock = (int)conn;

	// Send some data.
	char buf[MAX_CMD_LEN];
	memset(buf, 0, sizeof buf);
	snprintf(buf, sizeof buf,
			"action=set#table=%s#key=%s#value={%s}#metadata=%d!\n",
			table, key, value, md);

	if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0) {
		// Log server's response
		sprintf(message,
				"Server's response: '%s'\n",
				buf);
		logger(client_log,message);
		// Parse response
		struct protocol_arg_pair* args[MAX_ARG_NUM];
		extract_arg_from_line(args,buf);
		// Get status
		char status[MAX_ARG_VAL_LEN];
		get_arg_val(args,"status",status);
		if (strcmp(status,"0") == 0) {
			return 0;
		} else {
			char error[MAX_ARG_VAL_LEN];
			get_arg_val(args,"error",error);
			errno = error[0] - '0';
			return -1;
		}
	}
	errno = 7;
	return -1;
}


int storage_query(const char *table, const char *predicates, char **keys,
		const int max_keys, void *conn) {
	// Check parameters
	if (table == NULL
			|| predicates == NULL
			|| keys == NULL
			|| conn == NULL
			|| strlen(table) == 0
			|| strlen(predicates) == 0
			|| max_keys < 0) {
		errno = 1;
		return -1;
	}
	// Logger call
	sprintf(message,
			"Received a QUERY command with table:'%s' predicates:'%s' "\
			"max_keys:'%d'\n",
			table,
			predicates,
			max_keys);
	logger(client_log,message);
	// Connection is really just a socket file descriptor.
	int sock = (int)conn;

	// Send some data.
	char buf[MAX_CMD_LEN];
	memset(buf, 0, sizeof buf);
	snprintf(buf,sizeof(buf),
			"action=query#table=%s#max=%d#predicates={%s}!\n",
			table,max_keys,predicates);
	if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0) {
		// Log server's response

		sprintf(message,
			"Server's response: '%s'\n",
			buf);
		logger(client_log,message);

		// Parse response
		struct protocol_arg_pair* args[MAX_ARG_NUM];
		extract_arg_from_line(args,buf);

		// Get status
		char status[MAX_ARG_VAL_LEN];
		get_arg_val(args,"status",status);

		if (strcmp(status,"0") == 0) {
			char keys_temp[MAX_ARG_VAL_LEN],num_str[MAX_ARG_VAL_LEN];
			get_arg_val(args,"keys",keys_temp);
			get_arg_val(args,"num",num_str);

			int num = atoi(num_str);
			// get rid of the leading '{' and trailing '}'
			char keys_b[256];
			strncpy(keys_b,&keys_temp[1],strlen(keys_temp)-2);
			keys_b[strlen(keys_temp)-2] = '\0';
			char *p = strtok(keys_b,",");
			int k = 0;
			while (k < max_keys && k < num) {
				// extract each key
				keys[k] = (char*)malloc(MAX_KEY_LEN * sizeof(char));
				strcpy(keys[k],p);
				p = strtok(NULL,",");
				k++;
			}
			if (max_keys > num) {
				keys[num] = (char*)malloc(MAX_KEY_LEN);
				keys[num][0] = '\0';
			}
			return num;
		} else {
			char error[MAX_ARG_VAL_LEN];
			get_arg_val(args,"error",error);
			errno = error[0] - '0';
			return -1;
		}
	}
	errno = 7;
	return -1;
}



/**
 * @brief This is just a minimal stub implementation.  You should modify it 
 * according to your design.
 */
int storage_disconnect(void *conn)
{
	// Invalid connection
	if (conn == NULL) {
		errno = 1;
		return -1;
	}

	// Logger call
	logger(client_log,"Received a DISCONNECT command\n");

	// Cleanup
	int sock = (int)conn;
	close(sock);

	return 0;
}

