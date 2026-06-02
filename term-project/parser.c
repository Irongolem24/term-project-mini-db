#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

#define MAX_TOKENS 64

static int g_order_col_idx;
static int g_order_desc;

static int compare_rows(const void* a, const void* b) {
	Row* ra = *(Row**)a;
	Row* rb = *(Row**)b;
	int cmp = cell_compare(&ra->cells[g_order_col_idx], &rb->cells[g_order_col_idx]);
	return g_order_desc ? -cmp : cmp;
}

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
	if (count < 4) {
		printf("error: syntax is SELECT [DISTINCT] <cols> FROM <table> [WHERE ...] [ORDER BY col [ASC|DESC]] [LIMIT n]\n");
		return;
	}

	int distinct = 0;
	int col_start = 1;
	if (strcasecmp(tokens[1], "DISTINCT") == 0) {
		distinct = 1;
		col_start = 2;
	}

	int from_idx = -1;
	for (int i = col_start; i < count; i++) {
		if (strcasecmp(tokens[i], "from") == 0) {
			from_idx = i;
			break;
		}
	}
	if (from_idx < 0 || from_idx + 1 >= count) {
		printf("error: syntax is SELECT [DISTINCT] <cols> FROM <table> [WHERE ...] [ORDER BY col [ASC|DESC]] [LIMIT n]\n");
		return;
	}

	char* table_name = tokens[from_idx + 1];
	Table* t = table_find(db, table_name);
	if (t == NULL) {
		printf("error: table '%s' not found\n", table_name);
		return;
	}

	int limit = -1;
	int limit_idx = -1;
	for (int i = from_idx + 2; i < count; i++) {
		if (strcasecmp(tokens[i], "LIMIT") == 0 && i + 1 < count) {
			limit_idx = i;
			limit = atoi(tokens[i + 1]);
			break;
		}
	}

	int order_by_idx = -1;
	int order_col_idx = -1;
	int order_desc = 0;
	int ob_search_end = (limit_idx >= 0) ? limit_idx : count;
	for (int i = from_idx + 2; i < ob_search_end - 1; i++) {
		if (strcasecmp(tokens[i], "ORDER") == 0 && strcasecmp(tokens[i + 1], "BY") == 0) {
			order_by_idx = i;
			break;
		}
	}
	if (order_by_idx >= 0) {
		if (order_by_idx + 2 >= ob_search_end) {
			printf("error: ORDER BY requires a column name\n");
			return;
		}
		char* ob_col = tokens[order_by_idx + 2];
		for (int i = 0; i < t->col_count; i++) {
			if (strcasecmp(t->columns[i].name, ob_col) == 0) {
				order_col_idx = i;
				break;
			}
		}
		if (order_col_idx < 0) {
			printf("error: column '%s' not found\n", ob_col);
			return;
		}
		int desc_pos = order_by_idx + 3;
		if (desc_pos < ob_search_end && strcasecmp(tokens[desc_pos], "DESC") == 0)
			order_desc = 1;
	}

	int where_end = (order_by_idx >= 0) ? order_by_idx
	              : (limit_idx   >= 0) ? limit_idx
	              : count;

	int sel_cols[MAX_COLUMNS];
	int sel_count = 0;

	if (strcmp(tokens[col_start], "*") == 0) {
		for (int i = 0; i < t->col_count; i++) sel_cols[i] = i;
		sel_count = t->col_count;
	} else {
		char col_buf[1024] = "";
		for (int i = col_start; i < from_idx; i++)
			strncat_s(col_buf, sizeof(col_buf), tokens[i], _TRUNCATE);

		char* ctx;
		char* col_tok = strtok_s(col_buf, ",", &ctx);
		while (col_tok && sel_count < MAX_COLUMNS) {
			while (*col_tok == ' ') col_tok++;

			int found = -1;
			for (int i = 0; i < t->col_count; i++) {
				if (strcasecmp(t->columns[i].name, col_tok) == 0) {
					found = i;
					break;
				}
			}
			if (found < 0) {
				printf("error: column '%s' not found\n", col_tok);
				return;
			}
			sel_cols[sel_count++] = found;
			col_tok = strtok_s(NULL, ",", &ctx);
		}
	}

	WhereClause wc;
	wc.count = 0;
	for (int i = from_idx + 2; i < where_end; i++) {
		if (strcasecmp(tokens[i], "where") == 0) {
			if (!parse_where(t, tokens, i, where_end, &wc)) return;
			break;
		}
	}

	for (int i = 0; i < sel_count; i++)
		printf("%-20s", t->columns[sel_cols[i]].name);
	printf("\n--------------------------------------------\n");

	if (order_by_idx >= 0) {
		Row* matched[4096];
		int matched_count = 0;

		Row* r = t->rows;
		while (r != NULL) {
			if (row_matches_where(r, &wc) && matched_count < 4096)
				matched[matched_count++] = r;
			r = r->next;
		}

		g_order_col_idx = order_col_idx;
		g_order_desc    = order_desc;
		qsort(matched, matched_count, sizeof(Row*), compare_rows);

		Row* seen[1024];
		int seen_count = 0;
		int found = 0;
		for (int idx = 0; idx < matched_count; idx++) {
			if (limit >= 0 && found >= limit) break;
			Row* r = matched[idx];

			if (distinct) {
				int dup = 0;
				for (int s = 0; s < seen_count; s++) {
					int same = 1;
					for (int i = 0; i < sel_count; i++) {
						if (cell_compare(&r->cells[sel_cols[i]], &seen[s]->cells[sel_cols[i]]) != 0) {
							same = 0; break;
						}
					}
					if (same) { dup = 1; break; }
				}
				if (dup) continue;
				seen[seen_count++] = r;
			}

			for (int i = 0; i < sel_count; i++) {
				Cell* c = &r->cells[sel_cols[i]];
				if (c->type == DATA_INT)        printf("%-20d", c->int_val);
				else if (c->type == DATA_FLOAT) printf("%-20f", c->float_val);
				else if (c->type == DATA_TEXT)  printf("%-20s", c->text_val);
			}
			printf("\n");
			found++;
		}
		printf("(%d rows)\n", found);

	} else {
		Row* seen[1024];
		int seen_count = 0;
		int found = 0;

		Row* r = t->rows;
		while (r != NULL) {
			if (limit >= 0 && found >= limit) break;

			if (row_matches_where(r, &wc)) {
				if (distinct) {
					int dup = 0;
					for (int s = 0; s < seen_count; s++) {
						int same = 1;
						for (int i = 0; i < sel_count; i++) {
							if (cell_compare(&r->cells[sel_cols[i]], &seen[s]->cells[sel_cols[i]]) != 0) {
								same = 0; break;
							}
						}
						if (same) { dup = 1; break; }
					}
					if (dup) { r = r->next; continue; }
					seen[seen_count++] = r;
				}

				for (int i = 0; i < sel_count; i++) {
					Cell* c = &r->cells[sel_cols[i]];
					if (c->type == DATA_INT)        printf("%-20d", c->int_val);
					else if (c->type == DATA_FLOAT) printf("%-20f", c->float_val);
					else if (c->type == DATA_TEXT)  printf("%-20s", c->text_val);
				}
				printf("\n");
				found++;
			}
			r = r->next;
		}
		printf("(%d rows)\n", found);
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
		printf("error: syntax is DELETE FROM <table> WHERE <col> <op> <val>\n");
		return;
	}

	Table* t = table_find(db, tokens[2]);
	if (t == NULL) {
		printf("error: table '%s' not found\n", tokens[2]);
		return;
	}

	WhereClause wc;
	if (!parse_where(t, tokens, 3, count, &wc)) return;

	int deleted = rows_delete_where(t, &wc);

	if (deleted == 0) printf("error: no matching rows\n");
	else printf("Success (%d rows deleted)\n", deleted);
}

static void parse_update(Database* db, char* tokens[], int count) {
	if (count < 10 || strcasecmp(tokens[2], "set") != 0
		|| strcasecmp(tokens[6], "where") != 0) {
		printf("error: syntax is UPDATE <table> SET <col> = <val> WHERE ...\n");
		return;
	}

	Table* t = table_find(db, tokens[1]);
	if (t == NULL) {
		printf("error: table '%s' not found\n", tokens[1]);
		return;
	}

	int set_col_idx = -1;
	for (int i = 0; i < t->col_count; i++) {
		if (strcasecmp(t->columns[i].name, tokens[3]) == 0) {
			set_col_idx = i;
			break;
		}
	}
	if (set_col_idx < 0) {
		printf("error: column '%s' not found\n", tokens[3]);
		return;
	}

	char* val_str = tokens[5];
	Cell new_val;
	new_val.type = t->columns[set_col_idx].type;

	if (new_val.type == DATA_INT)        new_val.int_val   = atoi(val_str);
	else if (new_val.type == DATA_FLOAT) new_val.float_val = atof(val_str);
	else {
		if (*val_str == '\'') val_str++;
		int vlen = (int)strlen(val_str);
		if (vlen > 0 && val_str[vlen - 1] == '\'') val_str[vlen - 1] = '\0';
		new_val.text_val = val_str;
	}

	WhereClause wc;
	if (!parse_where(t, tokens, 6, count, &wc)) return;

	int updated = rows_update_where(t, set_col_idx, &new_val, &wc);

	if (updated == 0) printf("error: no matching rows\n");
	else printf("Success (%d rows updated)\n", updated);
}

static void parse_clear() {
#ifdef _WIN32
	system("cls");
#else
	system("clear");
#endif
}

static void parse_help() {
	printf("\n");
	printf("=== Mini-SQLite 명령어 도움말 ===\n");
	printf("\n");
	printf("[ 테이블 (DDL) ]\n");
	printf("  CREATE TABLE <name> (col TYPE [PRIMARY KEY], ...)\n");
	printf("      예: CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)\n");
	printf("      타입: INT / TEXT / FLOAT\n");
	printf("  DROP TABLE <name>            테이블 삭제\n");
	printf("  SHOW TABLES                  테이블 목록 출력\n");
	printf("  DESCRIBE <name>              컬럼 구조 출력\n");
	printf("\n");
	printf("[ 데이터 (DML) ]\n");
	printf("  INSERT INTO <name> VALUES (val, ...)\n");
	printf("      예: INSERT INTO users VALUES (1, 'Alice', 20)\n");
	printf("  SELECT [DISTINCT] <cols|*> FROM <name>\n");
	printf("         [WHERE <cond>] [ORDER BY <col> [ASC|DESC]] [LIMIT <n>]\n");
	printf("      예: SELECT * FROM users\n");
	printf("      예: SELECT name, age FROM users WHERE age >= 20 ORDER BY age DESC LIMIT 5\n");
	printf("  UPDATE <name> SET <col> = <val> WHERE <cond>\n");
	printf("      예: UPDATE users SET age = 21 WHERE id = 1\n");
	printf("  DELETE FROM <name> WHERE <cond>\n");
	printf("      예: DELETE FROM users WHERE id = 1\n");
	printf("\n");
	printf("[ WHERE 조건 ]\n");
	printf("  연산자: =  !=  <  >  <=  >=\n");
	printf("  결합:   AND / OR  (최대 4개 조건, 왼쪽->오른쪽 평가)\n");
	printf("      예: WHERE age > 20 AND name != 'Bob'\n");
	printf("\n");
	printf("[ 유틸리티 ]\n");
	printf("  HELP                         이 도움말 출력\n");
	printf("  CLEAR                        화면 지우기\n");
	printf("  EXIT (또는 QUIT)             종료 (자동 저장)\n");
	printf("\n");
	printf("문자열은 작은따옴표로 감쌉니다: 'Alice'\n");
	printf("명령어는 대소문자를 구분하지 않습니다.\n");
	printf("\n");
}

static int parse_where(Table* t, char* tokens[], int where_idx, int token_count, WhereClause* wc) {
	wc->count = 0;
	int i = where_idx + 1;

	while (i + 2 <= token_count - 1 && wc->count < MAX_CONDITIONS) {
		char* col_name = tokens[i];
		char* op_str   = tokens[i + 1];
		char* val_str  = tokens[i + 2];

		int col_idx = -1;
		for (int j = 0; j < t->col_count; j++) {
			if (strcasecmp(t->columns[j].name, col_name) == 0) {
				col_idx = j;
				break;
			}
		}
		if (col_idx < 0) {
			printf("error: column '%s' not found\n", col_name);
			return 0;
		}

		Operator op;
		if      (strcmp(op_str, "=")  == 0) op = OP_EQ;
		else if (strcmp(op_str, "!=") == 0) op = OP_NEQ;
		else if (strcmp(op_str, "<")  == 0) op = OP_LT;
		else if (strcmp(op_str, ">")  == 0) op = OP_GT;
		else if (strcmp(op_str, "<=") == 0) op = OP_LTE;
		else if (strcmp(op_str, ">=") == 0) op = OP_GTE;
		else {
			printf("error: unknown operator '%s'\n", op_str);
			return 0;
		}

		Cell val;
		val.type = t->columns[col_idx].type;
		if (val.type == DATA_INT)        val.int_val   = atoi(val_str);
		else if (val.type == DATA_FLOAT) val.float_val = atof(val_str);
		else {
			if (*val_str == '\'') val_str++;
			int vlen = (int)strlen(val_str);
			if (vlen > 0 && val_str[vlen - 1] == '\'') val_str[vlen - 1] = '\0';
			val.text_val = val_str;
		}

		wc->conds[wc->count].col_idx = col_idx;
		wc->conds[wc->count].op      = op;
		wc->conds[wc->count].val     = val;
		wc->count++;
		i += 3;

		if (i < token_count && wc->count < MAX_CONDITIONS) {
			if (strcasecmp(tokens[i], "AND") == 0) { wc->logic[wc->count - 1] = LOGIC_AND; i++; }
			else if (strcasecmp(tokens[i], "OR")  == 0) { wc->logic[wc->count - 1] = LOGIC_OR;  i++; }
			else break;
		} else {
			break;
		}
	}

	return wc->count > 0 ? 1 : 0;
}

static void dispatch(Database* db, char* tokens[], int count) {
	if (count == 0) return;

	char* cmd = tokens[0];

	if (strcasecmp(cmd, "EXIT") == 0
		|| strcasecmp(cmd, "QUIT") == 0) {
	}
	else if (strcasecmp(cmd, "HELP") == 0)       parse_help();
	else if (strcasecmp(cmd, "SHOW") == 0)       parse_show(db, tokens, count);
	else if (strcasecmp(cmd, "DESCRIBE") == 0)   parse_describe(db, tokens, count);
	else if (strcasecmp(cmd, "DROP") == 0)       parse_drop(db, tokens, count);
	else if (strcasecmp(cmd, "INSERT") == 0)     parse_insert(db, tokens, count);
	else if (strcasecmp(cmd, "SELECT") == 0)     parse_select(db, tokens, count);
	else if (strcasecmp(cmd, "CREATE") == 0)     parse_create(db, tokens, count);
	else if (strcasecmp(cmd, "DELETE") == 0)     parse_delete(db, tokens, count);
	else if (strcasecmp(cmd, "UPDATE") == 0)     parse_update(db, tokens, count);
	else if (strcasecmp(cmd, "CLEAR") == 0)		parse_clear();
	else    printf("error: unknown command '%s'\n", cmd);
}

void parse_command(Database* db, char* input) {
	char* tokens[MAX_TOKENS];
	int count = tokenize(input, tokens);
	dispatch(db, tokens, count);
}
