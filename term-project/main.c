#include <stdio.h>
#include <string.h>
#include "parser.h"

#ifdef _WIN32
    #define strcasecmp _stricmp
#endif

int main(void) {
    Database* db = db_create();
    char buf[1024];

    while (1) {
        printf("db> ");
        if (!fgets(buf, sizeof(buf), stdin)) break;
        if (strcasecmp(buf, "exit") == 0) break;
        parse_command(db, buf);
    }

    db_free(db);
    return 0;
}