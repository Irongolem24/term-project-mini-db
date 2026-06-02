#pragma once

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "db.h"

#define HT_BUCKETS 64

typedef struct HtEntry {
	Cell            key;
	Row*            row;
	struct HtEntry* next;
} HtEntry;

typedef struct HashTable {
	HtEntry** buckets;
	int       bucket_count;
	int       size;
} HashTable;

HashTable* ht_create(int bucket_count);
void       ht_insert(HashTable* ht, Cell* key, Row* row);
Row*       ht_find(HashTable* ht, Cell* key);
void       ht_delete(HashTable* ht, Cell* key);
void       ht_free(HashTable* ht);

#endif
