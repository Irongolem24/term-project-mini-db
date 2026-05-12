#pragma once

#ifndef STORAGE_H
#define STORAGE_H

#include "db.h"

void db_save(Database* db, const char* path);
void db_load(Database* db, const char* path);

#endif