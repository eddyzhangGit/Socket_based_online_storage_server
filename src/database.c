/*
 * database.c
 *
 *  Created on: 2014-02-06
 *      Author: choushuo
 */

#include <stdlib.h>
#include "database.h"

int init_tables(struct table** table_arr) {
	int k = 0;
	while (table_arr[k] != 0) {
		tables[k] = (struct data_table*)malloc(sizeof(struct data_table));
		strcpy(tables[k]->name,table_arr[k]->name);
		tables[k]->head = 0;
		tables[k]->col_count = table_arr[k]->col_count;
		int m;
		for (m=0; m<table_arr[k]->col_count; m++) {
			tables[k]->columns[m] =
					(struct data_column*)malloc(sizeof(struct data_column));
			strcpy(tables[k]->columns[m]->name,
					table_arr[k]->columns[m]->name);
			if (strcmp(table_arr[k]->columns[m]->type,"int") == 0) {
				tables[k]->columns[m]->type = INT;
			} else {
				tables[k]->columns[m]->type = CHAR;
				char* l = strchr(table_arr[k]->columns[m]->type,'[');
				char* r = strchr(table_arr[k]->columns[m]->type,']');
				char* num;
				strncpy(num,l+1,r-l-1);
				num[r-l-1] = '\0';
				int n = atoi(num);
				if (n<0) {
					return -1;
				}
				tables[k]->columns[m]->str_len = n;
			}
		}
		k++;
	}
	query_conditions[0] = 0;
	table_count = k;
	condition_count = 0;
	return 0;
}

struct data_table* find_table(char* table_name) {
	int k;
	// search through table array for specified table name
	for (k=0; k<table_count; k++) {
		if (strcmp(tables[k]->name,table_name) == 0) {
			// found, return pointer
			return tables[k];
		}
	}
	// not found, return null
	return 0;
}

struct data_entry* find_entry(struct data_table* table, char* search_key) {
	struct data_entry* cursor = table->head;
	// search through linked-list for specified key
	while (cursor != 0) {
		if (strcmp(cursor->key,search_key) == 0) {
			// found, return pointer
			return cursor;
		}
		cursor = cursor->next;
	}
	// not found, return null
	return 0;
}

int set_entry(struct data_table* table, char* mod_key, char mod_value[MAX_COLUMNS_PER_TABLE][MAX_VALUE_LEN], int metadata) {
	// if list is empty
	if (table->head == 0) {
		struct data_entry* entry = (struct data_entry*)malloc(sizeof(struct data_entry));
		strcpy(entry->key,mod_key);
		fill_entry_with_value(entry,mod_value,table->col_count);
		entry->metadata = 1;
		entry->next = 0;
		table->head = entry;
		return 0;
	}
	// if list is not empty
	struct data_entry* prev_cursor = table->head;
	struct data_entry* curr_cursor = table->head;
	// search through linked-list for specified key
	while (curr_cursor != 0) {
		if (strcmp(curr_cursor->key,mod_key) == 0) {
			// found, modify value
			if (metadata != 0 && metadata != curr_cursor->metadata) {
				// abort transaction
				return -1;
			}
			fill_entry_with_value(curr_cursor,mod_value,table->col_count);
			curr_cursor->metadata++;
			return 0;
		}
		prev_cursor = curr_cursor;
		curr_cursor = curr_cursor->next;
	}
	// key does not exist in linked-list, create new entry
	struct data_entry* entry = (struct data_entry*)malloc(sizeof(struct data_entry));
	strcpy(entry->key,mod_key);
	fill_entry_with_value(entry,mod_value,table->col_count);
	entry->metadata = 1;
	entry->next = 0;
	prev_cursor->next = entry;
	return 0;
}

int delete_entry(struct data_table* table, char* del_key) {
	struct data_entry* prev_cursor = table->head;
	struct data_entry* curr_cursor = table->head;
	// search through linked-list for specified key
	while (curr_cursor != 0) {
		if (strcmp(curr_cursor->key,del_key) == 0) {
			// found, delete entry
			prev_cursor = curr_cursor->next;
			free(curr_cursor);
			return 0;
		}
		prev_cursor = curr_cursor;
		curr_cursor = curr_cursor->next;
	}
	// not found, return -1
	return -1;
}

int get_col_index(struct data_table* table, char* col_name) {
	int k;
	for (k=0; k<table->col_count; k++) {
		if (strcmp(table->columns[k]->name,col_name) == 0) {
			return k;
		}
	}
	return -1;
}


void fill_entry_with_value(struct data_entry* entry, char value[MAX_COLUMNS_PER_TABLE][MAX_VALUE_LEN], int col_count) {
	int k=0;
	for (k=0; k<col_count; k++) {
		strcpy(entry->value[k],value[k]);
	}
}

void flush_query_params() {
	int k;
	for (k=0; k<condition_count; k++) {
		free(query_conditions[k]);
		query_conditions[k] = 0;
	}
	condition_count = 0;
}

int set_query_params(struct data_table* table,
		char col_n[MAX_COLNAME_LEN],
		char operand_t[MAX_VALUE_LEN],
		char comp_v[MAX_VALUE_LEN]) {
	int index = get_col_index(table,col_n);
	if (index == -1) {
		// column name not found
		return -1;
	}
	int k = 0;
	while (query_conditions[k] !=0 ) {
		k++;
	}
	enum operand_type op;
	if (strcmp(operand_t,"=") == 0) {
		op = EQUAL;
	} else if (strcmp(operand_t,">") == 0) {
		op = GREATER_THAN;
	} else if (strcmp(operand_t,"<") == 0) {
		op = LESS_THAN;
	} else {
		// unknown operand
		return -1;
	}
	if (table->columns[index]->type == INT && check_numeric(comp_v) != 0) {
		// did not input numerical value to compare int type
		return -1;
	}
	if (table->columns[index]->type == CHAR && op != EQUAL) {
		// incompatible operand for char type
		return -1;
	}
	query_conditions[k] = (struct query_condition*)
				malloc(sizeof(struct query_condition));
	query_conditions[k]->query_col_index = index;
	query_conditions[k]->query_operand = op;
	strcpy(query_conditions[k]->query_comp_val,comp_v);
	condition_count = k+1;
	return 0;
}

void query(struct data_table* table, char keys[MAX_RECORDS_PER_TABLE][MAX_KEY_LEN], int max_keys, int* keys_acquired) {
	int k = 0;
	struct data_entry* cursor = table->head;
	while (cursor != 0) {
		int result = check_query_match(table,cursor);
		if (result == 0) {
			strcpy(keys[k],cursor->key);
			k++;
		}
		cursor = cursor->next;
	}
	*keys_acquired = k;
}

int check_query_match(struct data_table* table, struct data_entry* entry) {
	int k, sum = 0;
	for (k=0; k<condition_count; k++) {
		struct query_condition* con = query_conditions[k];
		sum +=check_condition_match(table,entry,con);
	}
	return sum == 0 ? 0 : -1;
}

int check_condition_match(struct data_table* table,
		struct data_entry* entry,
		struct query_condition* con) {
	switch (table->columns[con->query_col_index]->type) {
		case INT:
		{
			int data_val = atoi(entry->value[con->query_col_index]);
			int other_val = atoi(con->query_comp_val);
			switch (con->query_operand) {
				case EQUAL:
				{
					if (data_val == other_val) {
						return 0;
					} else {
						return -1;
					}
				}
				case GREATER_THAN:
				{
					if (data_val > other_val) {
						return 0;
					} else {
						return -1;
					}
				}
				case LESS_THAN:
				{
					if (data_val < other_val) {
						return 0;
					} else {
						return -1;
					}
				}
				default:
				{
					// this will not happen
					return -1;
				}
			}
			break;
		}
		case CHAR:
		{
			// only possible operand is EQUAL
			if (strcmp(entry->value[con->query_col_index],con->query_comp_val) == 0) {
				return 0;
			} else {
				return -1;
			}
			break;
		}
		default:
		{
			// this will not happen
			return -1;
		}
	}
}




