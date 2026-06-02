#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "storage.h"

#define DB_FILE "data.txt"

#ifdef _WIN32
    #include <crtdbg.h>
    #define strcasecmp _stricmp
#endif

int main(void) {
#ifdef _WIN32
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
#endif

    Database* db = db_create();
    char buf[1024];

    db_load(db, DB_FILE);

    while (1) {
        printf("db> ");
        if (!fgets(buf, sizeof(buf), stdin)) break;
        if (strcasecmp(buf, "exit\n") == 0) break;
        parse_command(db, buf);
    }

    db_save(db, DB_FILE);
    db_free(db);
    _CrtDumpMemoryLeaks();
    return 0;
}
