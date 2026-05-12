#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

#ifdef _WIN32
	#define strcasecmp _stricmp
#endif

// 토크나이저
#define MAX_TOKENS 64

static int tokenize(char* input, char* tokens[]) {
	int count = 0;
	char* p = input;

	while (*p && count < MAX_TOKENS) {
		while (*p == ' ' || *p == '\t' || *p == '\n') p++;
		if (*p == '\0') break;

		if (*p == '(') {
			tokens[count++] = p;
			while (*p && *p != ')') p++;
			if (*p == ')') p++;
			if (*p) {
				*p = '\0';
				p++;
			}
		}
		else {
			tokens[count++] = p;
			while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '(') {
				p++;
			}
			if (*p) {
				*p = '\0';
				p++;
			}
		}
	}

	return count;
}

static void parse_show(Database* db, char* tokens[], int count) {
	if (count < 2 || strcasecmp(tokens[1], "tables") != 0) {
		printf("error: syntax is SHOW TABLES\n");
		return;
	}
	
	if (db->table_count == 0) {
		printf("(no tables)\n");
		return;
	}

	Table* t = db->tables;
	while (t != NULL) {
		printf("%s\n", t->name);
		t = t->next;
	}
	printf("(%d tables)\n", db->table_count);
}

static void parse_describe(Database* db, char* tokens[], int count) {
	if (count < 2) {
		printf("error: syntax is DESCRIBE <table>\n");
		return;
	}
	
	Table* t = table_find(db, tokens[1]);
	if (t == NULL) {
		printf("error: table '%s' not found\n", tokens[1]);
		return;
	}

	printf("%-20s %-10s %s\n", "name", "type", "PK");
	printf("--------------------------------------------\n");
	
	for (int i = 0; i < t->col_count; i++) {
		char* type_str;
		switch (t->columns[i].type) {
			case DATA_INT: type_str = "INT"; break;
			case DATA_FLOAT: type_str = "FLOAT"; break;
			case DATA_TEXT: type_str = "TEXT"; break;
			default: type_str = "?"; break;
		}

		printf("%-20s %-10s %s\n",
			t->columns[i].name,
			type_str,
			t->columns[i].is_pk ? "YES" : "NO");
	}
}

static void parse_drop(Database* db, char* tokens[], int count) {
	if (count < 3 || strcasecmp(tokens[1], "table") != 0) {
		printf("error: syntax is DROP TABLE <name>\n");
		return;
	}
	
	table_drop(db, tokens[2]);
}

static void parse_insert(Database* db, char* tokens[], int count) {
	if (count < 4 || strcasecmp(tokens[1], "into") != 0 || strcasecmp(tokens[3], "values") != 0) {
		printf("error: syntax is INSERT INTO <table> VALUES (val, ...)\n");
		return;
	}
	
	Table* t = table_find(db, tokens[2]);
	if (t == NULL) {
		printf("error: table '%s' not found\n", tokens[2]);
		return;
	}

	char val_str[1024];
	strncpy_s(val_str, sizeof(val_str), tokens[4], sizeof(val_str) - 1);

	char* start = val_str;
	if (*start == '(') start++;
	int len = (int)strlen(start);
	if (len > 0 && start[len - 1] == ')') start[len - 1] = '\0';

	Cell cells[MAX_COLUMNS];
	int val_count = 0;

	char* ctx;
	char* val = strtok_s(start, ",", &ctx);
	while (val && val_count < t->col_count) {
		while (*val == ' ') val++;

		Column* col = &t->columns[val_count];
		Cell* c = &cells[val_count];
		c->type = col->type;

		if (col->type == DATA_TEXT) {
			if (*val == '\'') val++;

			int vlen = (int)strlen(val);
			if (vlen > 0 && val[vlen - 1] == '\'') {
				val[vlen - 1] = '\0';
			}
			c->text_val = val;
		}
		else if (col->type == DATA_INT) {
			c->int_val = atoi(val);
		}
		else if (col->type == DATA_FLOAT) {
			c->float_val = atof(val);
		}

		val_count++;
		val = strtok_s(NULL, ",", &ctx);
	}

	if (val_count != t->col_count) {
		printf("error: expected %d values, got %d\n", t->col_count, val_count);
		return;
	}

	for (int i = 0; i < t->col_count; i++) {
		if (t->columns[i].is_pk && row_find_by_pk(t, &cells[i]) != NULL) {
			printf("error: duplicate PK\n");
			return;
		}
	}

	row_insert(t, cells);
	printf("Success\n");

}

static void parse_select(Database* db, char* tokens[], int count) {
	if (count < 4 || strcasecmp(tokens[2], "from") != 0) {
		printf("error: syntax is SELECT * FROM <table> [WHERE col = val]\n");
		return;
	}
	char* table_name = tokens[3];
	Table* t = table_find(db, table_name);
	if (t == NULL) {
		printf("error: table '%s' not found\n", table_name);
		return;
	}

	if (count == 4) {
		for (int i = 0; i < t->col_count; i++) {
			printf("%-20s", t->columns[i].name);
		}
		printf("\n");
		printf("--------------------------------------------\n");

		if (t->rows == NULL) {
			printf("(no rows)\n");
			return;
		}

		Row* r = t->rows;
		while (r != NULL) {
			for (int i = 0; i < t->col_count; i++) {
				Cell* c = &r->cells[i];
				if (c->type == DATA_INT) printf("%-20d", c->int_val);
				else if (c->type == DATA_FLOAT) printf("%-20f", c->float_val);
				else if (c->type == DATA_TEXT) printf("%-20s", c->text_val);
			}
			printf("\n");
			r = r->next;
		}
		printf("(%d rows)\n", t->row_count);
		
	}
	else if (count == 8 && strcasecmp(tokens[4], "WHERE") == 0) {
		// tokens[5]=col  tokens[6]='='  tokens[7]=val
		char* col = tokens[5];
		char* val = tokens[7];
		printf("(select from '%s' where %s = %s 실행)\n", table_name, col, val);
	}
	else {
		printf("error: invalid SELECT syntax\n");
	}
}

static void parse_create(Database* db, char* tokens[], int count) {
	if (count < 4 || strcasecmp(tokens[1], "TABLE") != 0) {
		printf("error: syntax is CREATE TABLE <name> (col TYPE [PRIMARY KEY], ...)\n");
		return;
	}

	char* table_name = tokens[2];
	Column cols[MAX_COLUMNS];
	int col_count = 0;

	char col_str[1024];
	strncpy_s(col_str, sizeof(col_str), tokens[3], sizeof(col_str) - 1);
	col_str[sizeof(col_str) - 1] = '\0';

	char* start = col_str;
	if (*start == '(') start++;

	int len = (int)strlen(start);
	if (len > 0 && start[len - 1] == ')') {
		start[len - 1] = '\0';
	}

	char* ctx1;
	char* col_def = strtok_s(start, ",", &ctx1);
	while (col_def && col_count < MAX_COLUMNS) {

		char* ctx2;
		char* col_name = strtok_s(col_def, " \t", &ctx2);
		char* type_str = strtok_s(NULL, " \t", &ctx2);
		char* pk1 = strtok_s(NULL, " \t", &ctx2);
		char* pk2 = strtok_s(NULL, " \t", &ctx2);

		if (!col_name || !type_str) {
			printf("error: invalid column definition");
			break;
		}

		DataType type;
		if (strcasecmp(type_str, "INT") == 0) type = DATA_INT;
		else if (strcasecmp(type_str, "TEXT") == 0) type = DATA_TEXT;
		else if (strcasecmp(type_str, "FLOAT") == 0) type = DATA_FLOAT;
		else {
			printf("error: unknown type '%s'\n", type_str);
			return;
		}

		int is_pk = (pk1 && strcasecmp(pk1, "PRIMARY") == 0 &&
			pk2 && strcasecmp(pk2, "KEY") == 0);

		strncpy_s(cols[col_count].name, MAX_NAME_LEN, col_name, MAX_NAME_LEN - 1);
		cols[col_count].name[MAX_NAME_LEN - 1] = '\0';
		cols[col_count].type = type;
		cols[col_count].is_pk = is_pk;
		col_count++;

		col_def = strtok_s(NULL, ",", &ctx1);
	}

	if (col_count == 0) {
		printf("error: no columns defined\n");
		return;
	}

	Table* t = table_create(db, table_name, cols, col_count);
	if (t) printf("Success\n");
}

static void parse_delete(Database* db, char* tokens[], int count) {
	if (count < 7 || strcasecmp(tokens[1], "from") != 0
		|| strcasecmp(tokens[3], "where") != 0) {
		printf("error: syntax is DELETE FROM <table> WHERE <col> = <val>\n");
		return;
	}

	Table* t = table_find(db, tokens[2]);
	if (t == NULL) {
		printf("error: table '%s' not found\n", tokens[2]);
		return;
	}

	char* col_name = tokens[4];
	int col_idx = -1;
	for (int i = 0; i < t->col_count; i++) {
		if (strcasecmp(t->columns[i].name, col_name) == 0) {
			col_idx = i;
			break;
		}
	}
	if (col_idx < 0) {
		printf("error: column '%s' not found\n", col_name);
		return;
	}

	char* val_str = tokens[6];
	Cell key;
	key.type = t->columns[col_idx].type;

	if (key.type == DATA_INT)        key.int_val = atoi(val_str);
	else if (key.type == DATA_FLOAT) key.float_val = atof(val_str);
	else {
		if (*val_str == '\'') val_str++;
		int vlen = (int)strlen(val_str);
		if (vlen > 0 && val_str[vlen - 1] == '\'') val_str[vlen - 1] = '\0';
		key.text_val = val_str;
	}

	row_delete(t, &key);
	printf("Success\n");
}

static void parse_clear() {
	#ifdef _WIN32
		system("cls");
	#else
		system("clear");
	#endif
}

static void dispatch(Database* db, char* tokens[], int count) {
	if (count == 0) return;

	char* cmd = tokens[0];

	if (strcasecmp(cmd, "EXIT") == 0
		|| strcasecmp(cmd, "QUIT") == 0) { /* main에서 처리 */
	}
	else if (strcasecmp(cmd, "HELP") == 0)       printf("(help 출력)\n");
	else if (strcasecmp(cmd, "SHOW") == 0)       parse_show(db, tokens, count);
	else if (strcasecmp(cmd, "DESCRIBE") == 0)   parse_describe(db, tokens, count);
	else if (strcasecmp(cmd, "DROP") == 0)       parse_drop(db, tokens, count);
	else if (strcasecmp(cmd, "INSERT") == 0)     parse_insert(db, tokens, count);
	else if (strcasecmp(cmd, "SELECT") == 0)     parse_select(db, tokens, count);
	else if (strcasecmp(cmd, "CREATE") == 0)     parse_create(db, tokens, count);
	else if (strcasecmp(cmd, "DELETE") == 0)    parse_delete(db, tokens, count);
	else if (strcasecmp(cmd, "CLEAR") == 0)		parse_clear();
	// UPDATE, DELETE, CREATE, SAVE, LOAD 는 동일한 패턴으로 추가
	else    printf("error: unknown command '%s'\n", cmd);
}

void parse_command(Database* db, char* input) {
	char* tokens[MAX_TOKENS];
	int count = tokenize(input, tokens);
	dispatch(db, tokens, count);
}