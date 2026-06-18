#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../db.h"

static int passed = 0;
static int failed = 0;

#define PASS(msg) do { printf("[PASS] %s\n", msg); passed++; } while(0)
#define FAIL(msg) do { printf("[FAIL] %s\n", msg); failed++; } while(0)
#define CHECK(expr, msg) do { if (expr) PASS(msg); else FAIL(msg); } while(0)

static Column make_col(const char* name, DataType type, int is_pk) {
    Column c;
    strncpy(c.name, name, MAX_NAME_LEN - 1);
    c.name[MAX_NAME_LEN - 1] = '\0';
    c.type = type;
    c.is_pk = is_pk;
    return c;
}

static void test_db_create(void) {
    printf("\n=== test_db_create ===\n");
    Database* db = db_create();
    CHECK(db != NULL, "db_create returns non-null");
    CHECK(db->table_count == 0, "initial table_count is 0");
    CHECK(db->tables == NULL, "initial tables list is NULL");
    db_free(db);
}

static void test_table_create_basic(void) {
    printf("\n=== test_table_create_basic ===\n");
    Database* db = db_create();

    Column cols[2];
    cols[0] = make_col("id",   DATA_INT,  1);
    cols[1] = make_col("name", DATA_TEXT, 0);

    Table* t = table_create(db, "users", cols, 2);

    CHECK(t != NULL, "table_create returns non-null");
    CHECK(db->table_count == 1, "table_count incremented to 1");
    CHECK(strcmp(t->name, "users") == 0, "table name is 'users'");
    CHECK(t->col_count == 2, "col_count is 2");
    CHECK(t->row_count == 0, "initial row_count is 0");
    CHECK(t->rows == NULL, "initial rows list is NULL");

    db_free(db);
}

static void test_table_create_columns(void) {
    printf("\n=== test_table_create_columns ===\n");
    Database* db = db_create();

    Column cols[2];
    cols[0] = make_col("id",   DATA_INT,  1);
    cols[1] = make_col("name", DATA_TEXT, 0);

    Table* t = table_create(db, "users", cols, 2);

    /* cols[0]: id INT PRIMARY KEY */
    CHECK(strcmp(t->columns[0].name, "id") == 0,   "cols[0].name == 'id'");
    CHECK(t->columns[0].type  == DATA_INT,          "cols[0].type == DATA_INT");
    CHECK(t->columns[0].is_pk == 1,                 "cols[0].is_pk == 1");

    /* cols[1]: name TEXT */
    CHECK(strcmp(t->columns[1].name, "name") == 0,  "cols[1].name == 'name'");
    CHECK(t->columns[1].type  == DATA_TEXT,         "cols[1].type == DATA_TEXT");
    CHECK(t->columns[1].is_pk == 0,                 "cols[1].is_pk == 0");

    db_free(db);
}

static void test_table_create_float_col(void) {
    printf("\n=== test_table_create_float_col ===\n");
    Database* db = db_create();

    Column cols[1];
    cols[0] = make_col("score", DATA_FLOAT, 0);

    Table* t = table_create(db, "scores", cols, 1);
    CHECK(t != NULL, "table with FLOAT column created");
    CHECK(t->columns[0].type == DATA_FLOAT, "cols[0].type == DATA_FLOAT");

    db_free(db);
}

static void test_table_create_duplicate(void) {
    printf("\n=== test_table_create_duplicate ===\n");
    Database* db = db_create();

    Column cols[1];
    cols[0] = make_col("id", DATA_INT, 1);

    table_create(db, "users", cols, 1);
    Table* dup = table_create(db, "users", cols, 1);

    CHECK(dup == NULL, "duplicate table_create returns NULL");
    CHECK(db->table_count == 1, "table_count stays 1 after duplicate");

    db_free(db);
}

static void test_table_find(void) {
    printf("\n=== test_table_find ===\n");
    Database* db = db_create();

    Column cols[1];
    cols[0] = make_col("id", DATA_INT, 1);
    table_create(db, "users", cols, 1);

    Table* found = table_find(db, "users");
    CHECK(found != NULL, "table_find returns table after create");
    CHECK(strcmp(found->name, "users") == 0, "found table has correct name");

    Table* missing = table_find(db, "orders");
    CHECK(missing == NULL, "table_find returns NULL for non-existent table");

    db_free(db);
}

static void test_table_create_multiple(void) {
    printf("\n=== test_table_create_multiple ===\n");
    Database* db = db_create();

    Column cols[1];
    cols[0] = make_col("id", DATA_INT, 1);

    table_create(db, "users",  cols, 1);
    table_create(db, "orders", cols, 1);
    table_create(db, "items",  cols, 1);

    CHECK(db->table_count == 3, "table_count is 3 after three creates");
    CHECK(table_find(db, "users")  != NULL, "table 'users' found");
    CHECK(table_find(db, "orders") != NULL, "table 'orders' found");
    CHECK(table_find(db, "items")  != NULL, "table 'items' found");

    db_free(db);
}

int main(void) {
    test_db_create();
    test_table_create_basic();
    test_table_create_columns();
    test_table_create_float_col();
    test_table_create_duplicate();
    test_table_find();
    test_table_create_multiple();

    printf("\n=== Result: %d passed, %d failed ===\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
