#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

// ---- 내부 헬퍼 ----

// PK 값을 해시값(unsigned long)으로 변환
static unsigned long hash_cell(Cell* c) {
	unsigned long h = 5381;   // djb2 시작값

	if (c->type == DATA_TEXT) {
		// 문자열: 글자 하나씩 누적 (djb2)
		const unsigned char* s = (const unsigned char*)c->text_val;
		while (*s) {
			h = h * 33 + *s;
			s++;
		}
	}
	else if (c->type == DATA_INT) {
		// 정수: 값 자체를 해시로 (음수도 unsigned 변환으로 안전)
		h = (unsigned long)c->int_val;
	}
	else { // DATA_FLOAT
		// 실수: 8바이트를 바이트 단위로 누적
		unsigned char* p = (unsigned char*)&c->float_val;
		for (size_t i = 0; i < sizeof(double); i++) {
			h = h * 33 + p[i];
		}
	}

	return h;
}

// 두 PK가 같은지 비교 (cell_compare는 db.c에 있고 같으면 0 반환)
static int key_equal(Cell* a, Cell* b) {
	return cell_compare(a, b) == 0;
}

// PK 값을 엔트리에 복사 (TEXT는 문자열을 별도로 _strdup)
static void key_copy(Cell* dst, Cell* src) {
	*dst = *src;
	if (src->type == DATA_TEXT && src->text_val != NULL) {
		dst->text_val = _strdup(src->text_val);
	}
}

// 엔트리가 들고 있던 키 메모리 해제 (TEXT만 해당)
static void key_free(Cell* c) {
	if (c->type == DATA_TEXT && c->text_val != NULL) {
		free(c->text_val);
		c->text_val = NULL;
	}
}

// ---- 공개 함수 ----

HashTable* ht_create(int bucket_count) {
	HashTable* ht = (HashTable*)malloc(sizeof(HashTable));

	// 버킷 배열을 calloc으로 0(=NULL) 초기화
	ht->buckets = (HtEntry**)calloc(bucket_count, sizeof(HtEntry*));
	ht->bucket_count = bucket_count;
	ht->size = 0;

	return ht;
}

// 버킷 수를 2배로 늘리고 모든 엔트리를 재배치 (rehash)
// 기존 HtEntry 노드를 그대로 재사용하므로 키 복사/해제는 없음
static void ht_resize(HashTable* ht) {
	int new_count = ht->bucket_count * 2;
	HtEntry** new_buckets = (HtEntry**)calloc(new_count, sizeof(HtEntry*));
	if (new_buckets == NULL) return;   // 메모리 부족 시 기존 상태 유지

	for (int i = 0; i < ht->bucket_count; i++) {
		HtEntry* e = ht->buckets[i];
		while (e != NULL) {
			HtEntry* next = e->next;
			int idx = (int)(hash_cell(&e->key) % new_count);  // 새 버킷 수로 재계산
			e->next = new_buckets[idx];
			new_buckets[idx] = e;
			e = next;
		}
	}

	free(ht->buckets);
	ht->buckets = new_buckets;
	ht->bucket_count = new_count;
}

void ht_insert(HashTable* ht, Cell* key, Row* row) {
	if (ht == NULL) return;

	// 1. 해시 계산 → 버킷 인덱스
	int idx = (int)(hash_cell(key) % ht->bucket_count);

	// 2. 새 엔트리 생성 + 키 복사
	HtEntry* e = (HtEntry*)malloc(sizeof(HtEntry));
	key_copy(&e->key, key);
	e->row = row;

	// 3. 버킷 맨 앞에 끼워넣기 (충돌 시 chaining)
	e->next = ht->buckets[idx];
	ht->buckets[idx] = e;
	ht->size++;

	// 4. load factor가 0.75를 넘으면 버킷을 2배로 확장 → 체인 길이를 짧게 유지 (O(1) 보장)
	if (ht->size > ht->bucket_count * 3 / 4) {
		ht_resize(ht);
	}
}

Row* ht_find(HashTable* ht, Cell* key) {
	if (ht == NULL) return NULL;

	// 1. 해시 계산 → 버킷 인덱스
	int idx = (int)(hash_cell(key) % ht->bucket_count);

	// 2. 그 버킷 안에서 key가 일치하는 엔트리 탐색
	HtEntry* e = ht->buckets[idx];
	while (e != NULL) {
		if (key_equal(&e->key, key)) {
			return e->row;
		}
		e = e->next;
	}
	return NULL;
}

void ht_delete(HashTable* ht, Cell* key) {
	if (ht == NULL) return;

	int idx = (int)(hash_cell(key) % ht->bucket_count);

	HtEntry* prev = NULL;
	HtEntry* cur = ht->buckets[idx];

	while (cur != NULL) {
		if (key_equal(&cur->key, key)) {
			// 버킷 linked list에서 분리
			if (prev == NULL) ht->buckets[idx] = cur->next;
			else prev->next = cur->next;

			key_free(&cur->key);  // TEXT 키 메모리 해제
			free(cur);
			ht->size--;
			return;
		}
		prev = cur;
		cur = cur->next;
	}
}

void ht_free(HashTable* ht) {
	if (ht == NULL) return;

	// 모든 버킷을 돌며 엔트리 전부 해제
	for (int i = 0; i < ht->bucket_count; i++) {
		HtEntry* e = ht->buckets[i];
		while (e != NULL) {
			HtEntry* next = e->next;
			key_free(&e->key);
			free(e);
			e = next;
		}
	}

	free(ht->buckets);
	free(ht);
}
