#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage.h"

#ifdef _WIN32
	#define strcasecmp _stricmp
#endif

void db_save(Database* db, const char* path) {
	FILE* f = fopen(path, "w");

	if (f == NULL) {
		printf("error: cannot open file '%s'", path);
		return;
	}

	Table* t = db->tables;
	while (t != NULL) {
		fprintf(f, "TABLE %s\n", t->name);

		fprintf(f, "SCHEMA ");
		for (int i = 0; i < t->col_count; i++) {
			char* type_str;
			if (t->columns[i].type == DATA_INT) type_str = "INT";
			else if (t->columns[i].type == DATA_FLOAT) type_str = "FLOAT";
			else type_str = "TEXT";

			fprintf(f, "%s %s%s", t->columns[i].name, type_str,
				t->columns[i].is_pk ? " PK" : "");

			if (i < t->col_count - 1) fprintf(f, ", ");
		}
		fprintf(f, "\n");

		Row* r = t->rows;
		while (r != NULL) {
			for (int i = 0; i < t->col_count; i++) {
				Cell* c = &r->cells[i];
				if (c->type == DATA_INT)
					fprintf(f, "%d", c->int_val);
				else if (c->type == DATA_FLOAT)
					fprintf(f, "%f", c->float_val);
				else
					fprintf(f, "'%s'", c->text_val);

				if (i < t->col_count - 1) fprintf(f, ",");
			}
			fprintf(f, "\n");
			r = r->next;
		}
		fprintf(f, "END TABLE\n\n");
		t = t->next;
		
	}

	fclose(f);
}

void db_load(Database* db, const char* path) {
	FILE* f = fopen(path, "r");
	if (f == NULL) {
		return;
	}

	char line[1024];
	Table* cur_table = NULL;

	while (fgets(line, sizeof(line), f) != NULL) {
		line[strcspn(line, "\n")] = '\0';

		if (strncmp(line, "TABLE ", 6) == 0) {
			char* table_name = line + 6;

			if (table_find(db, table_name) != NULL) {
				printf("warning: table '%s' already exists, skipping\n", table_name);
				cur_table = NULL;
				continue;
			}
			cur_table = table_create(db, table_name, NULL, 0);
		}
		else if (strncmp(line, "SCHEMA ", 7) == 0 && cur_table != NULL) {
			char schema_buf[1024];
			strncpy_s(schema_buf, sizeof(schema_buf), line + 7, sizeof(schema_buf) - 1);

			char* ctx1;
			char* col_def = strtok_s(schema_buf, ",", &ctx1);
			while (col_def && cur_table->col_count < MAX_COLUMNS) {
				while (*col_def == ' ') col_def++;

				char* ctx2;
				char* col_name = strtok_s(col_def, " ", &ctx2);
				char* type_str = strtok_s(NULL, " ", &ctx2);
				char* pk_flag = strtok_s(NULL, " ", &ctx2);

				if (!col_name || !type_str) break;

				Column* col = &cur_table->columns[cur_table->col_count];
				strncpy_s(col->name, MAX_NAME_LEN, col_name, MAX_NAME_LEN - 1);

				if (strcasecmp(type_str, "INT") == 0) col->type = DATA_INT;
				else if (strcasecmp(type_str, "FLOAT") == 0) col->type = DATA_FLOAT;
				else col->type = DATA_TEXT;

				col->is_pk = (pk_flag && strcasecmp(pk_flag, "PK") == 0);
				cur_table->col_count++;

				col_def = strtok_s(NULL, ",", &ctx1);
			}
		}
		else if (strncmp(line, "END TABLE", 9) == 0) {
			cur_table = NULL;
		}
		else if (cur_table != NULL && line[0] != '\0') {
			char row_buf[1024];
			strncpy_s(row_buf, sizeof(row_buf), line, sizeof(row_buf) - 1);

			Cell cells[MAX_COLUMNS];
			int val_count = 0;

			char* ctx;
			char* val = strtok_s(row_buf, ",", &ctx);
			while (val && val_count < cur_table->col_count) {
				Cell* c = &cells[val_count];
				c->type = cur_table->columns[val_count].type;

				if (c->type == DATA_TEXT) {
					if (*val == '\'') val++;
					int vlen = (int)strlen(val);
					if (vlen > 0 && val[vlen - 1] == '\'') val[vlen - 1] = '\0';

					c->text_val = val;
				}
				else if (c->type == DATA_INT) c->int_val = atoi(val);
				else if (c->type == DATA_FLOAT) c->float_val = atof(val);

				val_count++;
				val = strtok_s(NULL, ",", &ctx);
			}

			if (val_count == cur_table->col_count) {
				row_insert(cur_table, cells);
			}
		}
	}

	fclose(f);
}