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
	//todo: show 실행
}

static void parse_describe(Database* db, char* tokens[], int count) {
	if (count < 2) {
		printf("error: syntax is DESCRIBE <table>\n");
		return;
	}
	//todo: 실행
}

static void parse_drop(Database* db, char* tokens[], int count) {
	if (count < 3 || strcasecmp(tokens[1], "table") != 0) {
		printf("error: syntax is DROP TABLE <name>\n");
		return;
	}
	//todo: 실행
}

static void parse_insert(Database* db, char* tokens[], int count) {
	if (count < 4 || strcasecmp(tokens[1], "into") != 0 || strcasecmp(tokens[3], "values") != 0) {
		printf("error: syntax is INSERT INTO <table> VALUES (val, ...)\n");
		return;
	}
	//todo: 실행
}

static void parse_select(Database* db, char* tokens[], int count) {
	if (count < 4 || strcasecmp(tokens[2], "from") != 0) {
		printf("error: syntax is SELECT * FROM <table> [WHERE col = val]\n");
		return;
	}
	char* table_name = tokens[3];

	if (count == 4) {
		printf("(select all from '%s' 실행)\n", table_name);
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
	if (count < 4 || stricmp(tokens[1], "TABLE") != 0) {
		printf("error: syntax is CREATE TABLE <name> (col TYPE [PRIMARY KEY], ...)\n");
		return;
	}

	char* table_name = tokens[2];
	Column cols[MAX_COLUMNS];
	int col_count = 0;

	char col_str[1024];
	strncpy(col_str, tokens[3], sizeof(col_str) - 1);
	col_str[sizeof(col_str) - 1] = '\0';

	char* start = col_str;
	if (*start == '(') start++;

	int len = (int)strlen(start);
	if (len > 0 && start[len - 1] == ')') {
		start[len - 1] = '\0';
	}

	char* ctx1;
	char* col_def = strok(start, ",", &ctx1);

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
	else if (strcasecmp(cmd, "CREATE") == 0)     printf("todo");
	// UPDATE, DELETE, CREATE, SAVE, LOAD 는 동일한 패턴으로 추가
	else    printf("error: unknown command '%s'\n", cmd);
}

void parse_command(Database* db, char* input) {
	char* tokens[MAX_TOKENS];
	int count = tokenize(input, tokens);
	dispatch(db, tokens, count);
}