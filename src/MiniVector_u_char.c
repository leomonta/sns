#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// 

#include "MiniVector_u_char.h"

#define MiniVector MiniVector_u_char

#define GROW_RATE 2

MiniVector MiniVector_u_char_make(const size_t initial_count) {
	auto capacity = initial_count == 0 ? 10 : initial_count;

	MiniVector res = {
	    .data     = malloc(capacity * sizeof(u_char)),
	    .capacity = capacity * sizeof(u_char),
	    .count    = 0,
	};

	// copy elision
	return res;
}

void MiniVector_u_char_destroy(MiniVector *vec) {

	free(vec->data);

	// zero everythin
	vec->data     = nullptr;
	vec->capacity = 0;
	vec->count    = 0;
}

void MiniVector_u_char_grow(MiniVector *vec) {

	vec->data = realloc(vec->data, vec->capacity * GROW_RATE);
	vec->capacity *= GROW_RATE;
	// for why 2 and not 1.6 or 1.5
	// See video -> https://www.youtube.com/watch?v=GZPqDvG615k
	// essentially after 3 array being used in the same memory space, 2 performs sligthly better than 1.5 abd others
}

bool MiniVector_u_char_get(const MiniVector *vec, const size_t index, u_char* result) {

	if (index >= vec->count) {
		// invalid pos
		return false;
	}

	*result = *(vec->data + index);
	return true;
}


void MiniVector_u_char_set(MiniVector *vec, const size_t index, const u_char *element) {

	if (index < vec->count) {
		memcpy(vec->data + index, element, sizeof(u_char));
	}
}

void MiniVector_u_char_append(MiniVector *vec, const u_char *element) {
	if (vec->count == vec->capacity / sizeof(u_char)) {
		MiniVector_u_char_grow(vec);
	}

	memcpy(vec->data + vec->count, element, sizeof(u_char));

	++(vec->count);
}

void MiniVector_u_char_insert(MiniVector *vec, const size_t index, const u_char *element) {
	if (index >= vec->count) {
		// invalid position
		return;
	}

	if (vec->count == vec->capacity / sizeof(u_char)) {
		// i have to grow
		// I am doing double work here (in case realloc cannot extend the given pointer)
		// either realloc copies everything and then I move part of the array further OR
		// I move part of the array and then realloc copies everything
		// If I were to malloc new memory I would not incur in a double copy BUT i would miss out on a potential easy realloc
		// So the decision lies on the distribution of good realloc vs bad realloc
		// but I don't have the data to know this. So I'll just go with grow (realloc)
		MiniVector_u_char_grow(vec);
	}

	// god bless memmove
	memmove(vec->data + index + 1, vec->data + index, vec->count - index);

	// finally write the data
	MiniVector_u_char_set(vec, index, element);
}


void MiniVector_u_char_remove(MiniVector *vec, const size_t index) {

	// we cant just overwrite the position to erase when
	// index is out of bound, > count or
	// index is on the last index (since we would run the risk of copying garbage)
	if (index >= vec->count) {
		// nothing to delete
		return;
	}

	if (index < vec->count - 1) {
		// just copy over it, no need to do anything else really
		memcpy(vec->data + index, vec->data + index + 1, (vec->count - index - 1) * sizeof(u_char));
	}

	--vec->count;
}

#undef MiniVector
