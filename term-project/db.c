#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"

#ifdef _WIN32
	#define strcasecmp _stricmp
#endif

Database* db_create() {
	Database* db = (Database*)malloc(sizeof(Database));

	db->tables = NULL;
	db->table_count = 0;

	return db;
}

Table* table_find(Database* db, const char* name) {
	Table* t = db->tables;
	while (t != NULL) {
		if (strcasecmp(t->name, name) == 0) return t;
		t = t->next;
	}
	return NULL;
}

Table* table_create(
	Database* db,
	const char* name,
	Column* cols,
	int col_count
) {
	if (table_find(db, name) != NULL) {
		printf("error: table '%s' already exists\n", name);
		return NULL;
	}

	Table* t = (Table*)malloc(sizeof(Table));
	strncpy_s(t->name, MAX_NAME_LEN, name, MAX_NAME_LEN - 1);
	t->name[MAX_NAME_LEN - 1] = '\0';

	for (int i = 0; i < col_count; i++) {
		t->columns[i] = cols[i];
	}
	t->col_count = col_count;

	t->rows = NULL;
	t->row_count = 0;

	t->next = db->tables;
	db->tables = t;
	db->table_count++;

	return t;
}

static void cell_free(Cell* c) {
	if (c->type == DATA_TEXT && c->text_val != NULL) {
		free(c->text_val);
		c->text_val = NULL;
	}
}

static void row_free(Row* row, int col_count) {
	for (int i = 0; i < col_count; i++) {
		cell_free(&row->cells[i]);
	}
	free(row);
}

Row* row_insert(Table* t, Cell* cells) {
	Row* row = (Row*)malloc(sizeof(Row));

	for (int i = 0; i < t->col_count; i++) {
		row->cells[i] = cells[i];

		if (cells[i].type == DATA_TEXT && cells[i].text_val != NULL) {
			row->cells[i].text_val = _strdup(cells[i].text_val);
		}
	}

	row->next = t->rows;
	t->rows = row;
	t->row_count++;

	return row;
}

static int pk_match(Cell* a, Cell* b) {
	if (a->type != b->type) return 0;
	switch (a->type) {
		case DATA_INT:   return a->int_val == b->int_val;
		case DATA_FLOAT: return a->float_val == b->float_val;
		case DATA_TEXT:  return strcmp(a->text_val, b->text_val) == 0;
	}
	return 0;
}

static int pk_col_index(Table* t) {
	for (int i = 0; i < t->col_count;i++) {
		if (t->columns[i].is_pk) return i;
	}
	return -1;
}

Row* row_find_by_pk(Table* t, Cell* pk_val) {
	int pk_idx = pk_col_index(t);
	if (pk_idx < 0) return NULL;

	Row* r = t->rows;
	while (r != NULL) {
		if (pk_match(&r->cells[pk_idx], pk_val)) {
			return r;
		}
		r = r->next;
	}
	return NULL;
}

void row_delete(Table* t, Cell* pk_val) {
	int pk_idx = pk_col_index(t);
	if (pk_idx < 0) return;

	Row* prev = NULL;
	Row* cur = t->rows;

	while (cur != NULL) {
		if (pk_match(&cur->cells[pk_idx], pk_val)) {
			if (prev == NULL) t->rows = cur->next;
			else prev->next = cur->next;

			row_free(cur, t->col_count);
			t->row_count--;
			return;
		}
		prev = cur;
		cur = cur->next;
	}
	printf("error: row not found\n");
}

void table_drop(Database* db, const char* name) {
	Table* prev = NULL;
	Table* cur = db->tables;

	while (cur != NULL) {
		if (strcasecmp(cur->name, name) == 0) {
			Row* r = cur->rows;
			while (r != NULL) {
				Row* next = r->next;
				row_free(r, cur->col_count);
				r = next;
			}
			if (prev == NULL) db->tables = cur->next;
			else prev->next = cur->next;

			free(cur);
			db->table_count--;
			return;
		}
		prev = cur;
		cur = cur->next;
	}
	printf("error: table '%s' not found\n", name);
}

void db_free(Database* db) {
	Table* t = db->tables;
	while (t != NULL) {
		Table* next = t->next;
		table_drop(db, t->name);
		t = next;
	}
	free(db);
}
