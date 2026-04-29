#include <stdio.h>
#include "parser.h"

int main(void) {
    Database* db = db_create();
    char buf[1024];

    while (1) {
        printf("db> ");
        if (!fgets(buf, sizeof(buf), stdin)) break;
        if (_strnicmp(buf, "exit", 4) == 0) break;
        parse_command(db, buf);
    }

    db_free(db);
    return 0;
}