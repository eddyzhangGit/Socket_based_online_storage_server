/*
 * parse_utils.h
 *
 *  Created on: 2014-03-08
 *      Author: choushuo
 */

#ifndef PARSE_UTILS_H_
#define PARSE_UTILS_H_

#include "storage.h"
#include <stdio.h>
#include <string.h>

// return length of chunk
int get_next_text_chunk(char* source, char sep_char, char chunk[256]);

// delete leading and trailing white-spaces in before, store in after
void delete_leading_trailing_spaces(char before[256], char after[256]);

// check if input is numeric
// return 0 if numeric, 1 if not
int check_numeric(char* input);

// parse predicates
int parse_predicates(char chunk[1024],
		char col_name[MAX_COLNAME_LEN],
		char operand[1],
		char val[MAX_VALUE_LEN]);


#endif /* PARSE_UTILS_H_ */
