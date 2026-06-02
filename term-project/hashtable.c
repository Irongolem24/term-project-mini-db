#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

static unsigned long hash_cell(Cell* c) {
	unsigned long h = 5381;

	if (c->type == DATA_TEXT) {
		const unsigned char* s = (const unsigned char*)c->text_val;
		while (*s) {
			h = h * 33 + *s;
			s++;
		}
	}
	else if (c->type == DATA_INT) {
		h = (unsigned long)c->int_val;
	}
	else {
		unsigned char* p = (unsigned char*)&c->float_val;
		for (size_t i = 0; i < sizeof(double); i++) {
			h = h * 33 + p[i];
		}
	}

	return h;
}

static int key_equal(Cell* a, Cell* b) {
	return cell_compare(a, b) == 0;
}

static void key_copy(Cell* dst, Cell* src) {
	*dst = *src;
	if (src->type == DATA_TEXT && src->text_val != NULL) {
		dst->text_val = _strdup(src->text_val);
	}
}

static void key_free(Cell* c) {
	if (c->type == DATA_TEXT && c->text_val != NULL) {
		free(c->text_val);
		c->text_val = NULL;
	}
}

HashTable* ht_create(int bucket_count) {
	HashTable* ht = (HashTable*)malloc(sizeof(HashTable));

	ht->buckets = (HtEntry**)calloc(bucket_count, sizeof(HtEntry*));
	ht->bucket_count = bucket_count;
	ht->size = 0;

	return ht;
}

static void ht_resize(HashTable* ht) {
	int new_count = ht->bucket_count * 2;
	HtEntry** new_buckets = (HtEntry**)calloc(new_count, sizeof(HtEntry*));
	if (new_buckets == NULL) return;

	for (int i = 0; i < ht->bucket_count; i++) {
		HtEntry* e = ht->buckets[i];
		while (e != NULL) {
			HtEntry* next = e->next;
			int idx = (int)(hash_cell(&e->key) % new_count);
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

	int idx = (int)(hash_cell(key) % ht->bucket_count);

	HtEntry* e = (HtEntry*)malloc(sizeof(HtEntry));
	key_copy(&e->key, key);
	e->row = row;

	e->next = ht->buckets[idx];
	ht->buckets[idx] = e;
	ht->size++;

	if (ht->size > ht->bucket_count * 3 / 4) {
		ht_resize(ht);
	}
}

Row* ht_find(HashTable* ht, Cell* key) {
	if (ht == NULL) return NULL;

	int idx = (int)(hash_cell(key) % ht->bucket_count);

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
			if (prev == NULL) ht->buckets[idx] = cur->next;
			else prev->next = cur->next;

			key_free(&cur->key);
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
