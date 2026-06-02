#pragma once

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "db.h"   // Cell, Row 타입 사용

#define HT_BUCKETS 64   // 버킷 개수 (충돌은 chaining으로 처리)

// 버킷 안의 한 노드 — PK(key)와 해당 Row를 묶어서 보관
typedef struct HtEntry {
	Cell            key;   // PK 값의 복사본 (TEXT는 _strdup로 별도 복사)
	Row*            row;   // 실제 Row를 가리키는 포인터
	struct HtEntry* next;  // 같은 버킷에서 충돌 시 다음 노드
} HtEntry;

// 해시테이블 본체 — 버킷 포인터 배열
typedef struct HashTable {
	HtEntry** buckets;      // 버킷 배열 (각 원소는 HtEntry linked list의 head)
	int       bucket_count; // 버킷 개수
	int       size;         // 현재 저장된 엔트리 수
} HashTable;

HashTable* ht_create(int bucket_count);
void       ht_insert(HashTable* ht, Cell* key, Row* row);
Row*       ht_find(HashTable* ht, Cell* key);
void       ht_delete(HashTable* ht, Cell* key);
void       ht_free(HashTable* ht);

#endif
