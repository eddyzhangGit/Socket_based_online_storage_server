/**
 * @file
 * @brief This file declares various utility functions that are
 * can be used by the storage server and client library.
 */

#ifndef	UTILS_H
#define UTILS_H

#include <stdio.h>
#include "storage.h"


/**
 * @brief Any lines in the config file that start with this character 
 * are treated as comments.
 */
static const char CONFIG_COMMENT_CHAR = '#';

/**
 * @brief The max length in bytes of a command from the client to the server.
 */
#define MAX_CMD_LEN (1024 * 8)

/**
 * @brief A macro to log some information.
 *
 * Use it like this:  LOG(("Hello %s", "world\n"))
 *
 * Don't forget the double parentheses, or you'll get weird errors!
 */
#define LOG(x)  {printf x; fflush(stdout);}

/**
 * @brief A macro to output debug information.
 * 
 * It is only enabled in debug builds.
 */
#ifdef NDEBUG
#define DBG(x)  {}
#else
#define DBG(x)  {printf x; fflush(stdout);}
#endif

/**
 * @brief A struct to store config parameters.
 */
struct config_params {
	/// The hostname of the server.
	char server_host[MAX_HOST_LEN];

	/// The listening port of the server.
	int server_port;

	/// The storage server's username
	char username[MAX_USERNAME_LEN];

	/// The storage server's encrypted password
	char password[MAX_ENC_PASSWORD_LEN];

	/// List of table names
	struct table* tables[MAX_TABLES];

	// Concurrency for multiple clients
	int concurrency;

	/// The directory where tables are stored.
//	char data_directory[MAX_PATH_LEN];
};

struct table {
	char name[MAX_TABLE_LEN];
	struct column* columns[MAX_COLUMNS_PER_TABLE];
	int col_count;
};
struct column {
	char name[MAX_COLNAME_LEN];
	char type[30];
};

/**
 * @brief Exit the program because a fatal error occured.
 *
 * @param msg The error message to print.
 * @param code The program exit return value.
 */
static inline void die(char *msg, int code)
{
	printf("%s\n", msg);
	exit(code);
}

/**
 * @brief Keep sending the contents of the buffer until complete.
 * @return Return 0 on success, -1 otherwise.
 *
 * The parameters mimic the send() function.
 */
int sendall(const int sock, const char *buf, const size_t len);

/**
 * @brief Receive an entire line from a socket.
 * @return Return 0 on success, -1 otherwise.
 */
int recvline(const int sock, char *buf, const size_t buflen);

/**
 * @brief Read and load configuration parameters.
 *
 * @param config_file The name of the configuration file.
 * @param params The structure where config parameters are loaded.
 * @return Return 0 on success, -1 otherwise.
 */
int read_config(const char *config_file, struct config_params *params);

/**
 * @brief Generates a log message.
 * 
 * @param file The output stream
 * @param message Message to log.
 */
void logger(FILE *file, char *message);

/**
 * @brief Default two character salt used for password encryption.
 */
#define DEFAULT_CRYPT_SALT "xx"

/**
 * @brief Generates an encrypted password string using salt CRYPT_SALT.
 * 
 * @param passwd Password before encryption.
 * @param salt Salt used to encrypt the password. If NULL default value
 * DEFAULT_CRYPT_SALT is used.
 * @return Returns encrypted password.
 */
char *generate_encrypted_password(const char *passwd, const char *salt);



// Logging parameters
#define LOGGING 0
#define MAX_LOG_FILE_NAME_LEN 32
#define MAX_MESSAGE_LEN 1024

// Log message
#define TIME_STAMP_TEMPLATE "[%Y/%m/%d %H:%M:%S]"
char message[MAX_MESSAGE_LEN];

// Client-side logging
#define CLIENT_LOG_FILE_NAME "Client-%Y-%m-%d-%H-%M-%S.log"
FILE* client_log;
void open_client_log();
void close_client_log();

// Server-side logging
#define SERVER_LOG_FILE_NAME "Server-%Y-%m-%d-%H-%M-%S.log"
FILE* server_log;
void open_server_log();
void close_server_log();

// Time information extractor
#include <time.h>
struct tm* get_time_info();


/**
 * Communication protocol between client and server
 */
#define MAX_ARG_NUM 8
#define MAX_ARG_NAME_LEN 16
#define MAX_ARG_VAL_LEN 800
#define TERMINATE_CHAR '!'

struct protocol_arg_pair {
	char arg_name[MAX_ARG_NAME_LEN];
	char arg_val[MAX_ARG_VAL_LEN];
} protocol_arg_pair;

void extract_arg_from_line(
		struct protocol_arg_pair* args[MAX_ARG_NUM],
		char line[MAX_CMD_LEN]);
int	get_arg_val(
		struct protocol_arg_pair* args[MAX_ARG_NUM],
		char* name,
		char value[MAX_ARG_VAL_LEN]);



/**
 * Mass SET with text file
 */
int extract_kv_from_file(char* file_name,
		char keys[MAX_RECORDS_PER_TABLE][MAX_KEY_LEN],
		char values[MAX_RECORDS_PER_TABLE][MAX_VALUE_LEN]);

#endif
