/*
 * parse_utils.c
 *
 *  Created on: 2014-03-08
 *      Author: choushuo
 */


#include "parse_utils.h"

int get_next_text_chunk(char* source, char sep_char, char chunk[256]) {
	if (source[0] == '\0') {
		strcpy(chunk,"");
		return 0;
	}
	int k = 0;
	while (source[k] != sep_char && source[k] != '\0') {
		k++;
	}
	strncpy(chunk,source,k);
	chunk[k] = '\0';
	return k;
}

void delete_leading_trailing_spaces(char before[256], char after[256]) {
	char* p = &before[0];
	char* end;
	while (*p == ' ') {
		p++;
	}
	if (*p == '\0') {
		// all spaces
		strcpy(after,p);
		return;
	}
	end = &before[strlen(before) - 1];
	while (end > p && *end == ' ') {
		end--;
	}
	*(end+1) = '\0';
	strcpy(after,p);
	return;
}

int check_numeric(char* input) {
	char* allowed_characters = "0123456789";
	char* p = input;
	// first char can be either a '-' or a numeric char
	if (strchr(allowed_characters,*p) == NULL && *p != '-') {
		return 1;
	}
	// check remaining chars
	p++;
	while (*p != '\0') {
		if (strchr(allowed_characters,*p) == NULL) {
			return 1;
		}
		p++;
	}
	return 0;
}

int parse_predicates(char chunk[1024],
		char col_name[MAX_COLNAME_LEN],
		char operand[1],
		char val[MAX_VALUE_LEN]) {
	char* head = &chunk[0];
	char* tail = &chunk[0];

		// copy col_name
		while (*head != ' '
				&& *head != '>'
				&& *head != '<'
				&& *head != '='
				&& *head != '\0') {
			head++;
		}
		if (head == tail || *head == '\0') {
			return -1;
		}
		strncpy(col_name,tail,head-tail);
		col_name[head-tail] = '\0';
		tail = head + 1;
		// copy operand
		while (*head != '>'
				&& *head != '<'
				&& *head != '='
				&& *head != '\0') {
			head++;
		}\
		if (*head == '\0') {
			return -1;
		}
		strncpy(operand,head,1);
		operand[1] = '\0';
		tail = head + 1;
		// copy val
		char val_buff[MAX_VALUE_LEN];
		strcpy(val_buff,tail);
		delete_leading_trailing_spaces(val_buff,val);
		if (val[0] == '\0') {
			return -1;
		}

	return 0;
}


