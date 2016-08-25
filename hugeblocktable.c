#include "hugeblocktable.h"

#include <assert.h>

QCGC_STATIC size_t bucket(object_t *object);

void qcgc_hbtable_initialize(void) {
	hbtable.mark_flag_ref = false;
	for (size_t i = 0; i < QCGC_HBTABLE_BUCKETS; i++) {
		hbtable.bucket[i] = qcgc_hbbucket_create(4);
	}
}

void qcgc_hbtable_destroy(void) {
	for (size_t i = 0; i < QCGC_HBTABLE_BUCKETS; i++) {
		free(hbtable.bucket[i]);
	}
}

void qcgc_hbtable_insert(object_t *object) {
	size_t i = bucket(object);
	hbtable.bucket[i] = qcgc_hbbucket_add(hbtable.bucket[i],
			(struct hbtable_entry_s) {
				.object = object,
				.mark_flag = !hbtable.mark_flag_ref});
}

void qcgc_hbtable_mark(object_t *object) {
	hbbucket_t *b = hbtable.bucket[bucket(object)];
	size_t count = b->count;
	for (size_t i = 0; i < count; i++) {
		if (b->items[i].object == object) {
			b->items[i].mark_flag = hbtable.mark_flag_ref;
			return;
		}
	}
#if CHECKED
	assert(false);
#endif
}

void qcgc_hbtable_sweep(void) {
	for (size_t i = 0; i < QCGC_HBTABLE_BUCKETS; i++) {
		hbbucket_t *b = hbtable.bucket[i];
		size_t j = 0;
		while(j < b->count) {
			if (b->items[j].mark_flag != hbtable.mark_flag_ref) {
				// White object
				// FIXME: Leaks object (currently not clear how it will
				// be allocated)
				b = qcgc_hbbucket_remove_index(b, j);
			} else {
				// Black object
				j++;
			}
		}
		hbtable.bucket[i] = b;
	}
	hbtable.mark_flag_ref = !hbtable.mark_flag_ref;
}

QCGC_STATIC size_t bucket(object_t *object) {
	return ((uintptr_t) object >> (QCGC_ARENA_SIZE_EXP)) % QCGC_HBTABLE_BUCKETS;
}