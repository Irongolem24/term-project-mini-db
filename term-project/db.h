#pragma once

#ifndef DB_H
#define DB_H

#define MAX_COLUMNS 8
#define MAX_NAME_LEN 64

typedef enum {
	DATA_INT,
	DATA_FLOAT,
	DATA_TEXT
} DataType;

typedef struct {
	DataType type;
	union {
		int int_val;
		double float_val;
		char* text_val;
	};
} Cell;

typedef struct Row{
	Cell cells[MAX_COLUMNS];
	struct Row* next;
} Row;

typedef struct {
	char name[MAX_NAME_LEN];
	DataType type;
	int is_pk;
} Column;

typedef struct Table {
	char name[MAX_NAME_LEN];
	Column columns[MAX_COLUMNS];
	int col_count;
	Row* rows;
	int row_count;
	struct Table* next;
} Table;

typedef struct {
	Table* tables;
	int table_count;
} Database;

Database* db_create(void);
void db_free(Database* db);

Table* table_create(Database* db, const char* name, Column* cols, int col_count);
Table* table_find(Database* db, const char* name);
void table_drop(Database* db, const char* name);

Row* row_insert(Table* t, Cell* cells);
Row* row_find_by_pk(Table* t, Cell* pk_val);
void row_delete(Table* t, Cell* pk_val);

#endif

