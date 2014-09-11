/*
 * database.h
 *
 *  Created on: 2014-02-06
 *      Author: choushuo
 */

#ifndef DATABASE_H_
#define DATABASE_H_

#include "storage.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>

/**
 * Number of tables that actually exist
 */
int table_count;

/**
 * An array of tables in the storage system
 */
struct data_table* tables[MAX_TABLES];

/**
 * A struct that represents a table with its name and head pointed of linked-list
 */
struct data_table {
	char name[MAX_TABLE_LEN];
	int col_count;
	struct data_column* columns[MAX_COLUMNS_PER_TABLE];
	struct data_entry* head;
};

/**
 * Enumeration for column type
 */
enum col_type {INT,CHAR};

/**
 * A struct that represents a column of a table
 */
struct data_column {
	char name[MAX_COLNAME_LEN];
	int str_len; // only applicable to char[] type
	enum col_type type;
};

/**
 * A struct that represents a node in a linked-list
 */
struct data_entry {
	char key[MAX_KEY_LEN];
	char value[MAX_COLUMNS_PER_TABLE][MAX_VALUE_LEN];
	int metadata;
	struct data_entry* next;
};


/**
 * Initialize table array
 */
int init_tables(struct table** table_arr);

/**
 * Find data_table pointer with table name
 * Return a pointer if found, 0 if not found
 */
struct data_table* find_table(char* table_name);

/**
 * Get entry from table
 * Return a pointer if found, 0 if not found
 */
struct data_entry* find_entry(struct data_table* table, char* search_key);

/**
 * Insert/modify entry to/in table
 * Return -1 if failed, 0 if successful
 */
int set_entry(struct data_table* table, char* mod_key, char mod_value[MAX_COLUMNS_PER_TABLE][MAX_VALUE_LEN], int metadata);

/**
 * Delete entry from table
 * Return -1 if failed, 0 if successful
 */
int delete_entry(struct data_table* table, char* del_key);

/**
 * Get column's index number in a table
 * Return -1 if column name not found
 */
int get_col_index(struct data_table* table, char* col_name);


// helper function
void fill_entry_with_value(struct data_entry* entry, char value[MAX_COLUMNS_PER_TABLE][MAX_VALUE_LEN], int col_count);




/*
 * Query
 */


// types of operand available
enum operand_type {GREATER_THAN,EQUAL,LESS_THAN};
// struct that represents a query condition
struct query_condition {
	int query_col_index;
	enum operand_type query_operand;
	char query_comp_val[MAX_VALUE_LEN];
};
// array of query conditions
struct query_condition* query_conditions[MAX_COLUMNS_PER_TABLE];
// number of valid conditions
int condition_count;


// delete previously set query parameters
void flush_query_params();

// program query parameters
// return 0 if parameters are acceptable, else return -1
int set_query_params(struct data_table* table,
		char col_n[MAX_COLNAME_LEN],
		char operand_t[MAX_VALUE_LEN],
		char comp_v[MAX_VALUE_LEN]);

// query the table, fill keys array with keys that meet query conditions
// should only be used after set_query_params is called
void query(struct data_table* table, char keys[MAX_RECORDS_PER_TABLE][MAX_KEY_LEN], int max_keys, int* keys_acquired);

// check if an entry matches the query
// return 0 if matches, else return -1
int check_query_match(struct data_table* table, struct data_entry* entry);

// check if an entry matches a query condition
// return 0 if matches, else return -1
int check_condition_match(struct data_table* table,
		struct data_entry* entry,
		struct query_condition* con);


#endif /* DATABASE_H_ */
