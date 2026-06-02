#pragma once

#ifndef DB_H
#define DB_H

#ifndef _WIN32
#define strncpy_s(dest, destsz, src, count) strncpy(dest, src, count)
#define strtok_s strtok_r
#define _strdup strdup
#endif

#define MAX_COLUMNS 8
#define MAX_NAME_LEN 64

#define MAX_CONDITIONS 4

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

struct HashTable;

typedef struct Table {
	char name[MAX_NAME_LEN];
	Column columns[MAX_COLUMNS];
	int col_count;
	Row* rows;
	int row_count;
	struct HashTable* ht;
	struct Table* next;
} Table;

typedef struct {
	Table* tables;
	int table_count;
} Database;

typedef enum {
	OP_EQ,   // =
	OP_NEQ,  // !=
	OP_LT,   // <
	OP_GT,   // >
	OP_LTE,  // <=
	OP_GTE   // >=
} Operator;

typedef enum {
	LOGIC_AND,
	LOGIC_OR
} LogicOp;

typedef struct {
	int      col_idx;
	Operator op;
	Cell     val;
} Condition;

typedef struct {
	Condition conds[MAX_CONDITIONS];
	LogicOp   logic[MAX_CONDITIONS - 1];
	int       count;
} WhereClause;

Database* db_create(void);
void db_free(Database* db);

Table* table_create(Database* db, const char* name, Column* cols, int col_count);
Table* table_find(Database* db, const char* name);
void table_drop(Database* db, const char* name);

Row* row_insert(Table* t, Cell* cells);
Row* row_find_by_pk(Table* t, Cell* pk_val);
void row_delete(Table* t, Cell* pk_val);
int cell_compare(Cell* a, Cell* b);
int row_matches_where(Row* r, WhereClause* wc);
int rows_delete_where(Table* t, WhereClause* wc);
int rows_update_where(Table* t, int set_col_idx, Cell* new_val, WhereClause* wc);

#endif
