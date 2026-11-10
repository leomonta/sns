#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"

template <T>

#include "MiniVector_#T#.h"

#define MiniVector MiniVector_#T#

#define GROW_RATE 2

ERRFUNC MiniVector_#T#_make(const size_t initial_count, MiniVector *vec) {
	auto capacity = initial_count == 0 ? 10 : initial_count;

	*vec = (MiniVector){
	    .data     = malloc(capacity * sizeof(T)),
	    .capacity = capacity * sizeof(T),
	    .count    = 0,
	};
	TEST_ALLOC(vec->data);

	// copy elision
	return NO_ERROR;
}

ERRFUNC MiniVector_#T#_destroy(MiniVector *vec) {

	free(vec->data);

	// zero everythin
	vec->data     = nullptr;
	vec->capacity = 0;
	vec->count    = 0;

	return NO_ERROR;
}

ERRFUNC MiniVector_#T#_grow(MiniVector *vec) {

	vec->data = realloc(vec->data, vec->capacity * GROW_RATE);
	TEST_ALLOC(vec->data);
	vec->capacity *= GROW_RATE;
	// for why 2 and not 1.6 or 1.5
	// See video -> https://www.youtube.com/watch?v=GZPqDvG615k
	// essentially after 3 array being used in the same memory space, 2 performs sligthly better than 1.5 abd others

	return NO_ERROR;
}

ERRFUNC MiniVector_#T#_get(const MiniVector *vec, const size_t index, T *result) {

	if (index >= vec->count) {
		llog(LOG_WARNING, "[MINIVECTOR] %ld >= %ld Unreachable position\n", index, vec->count);
		return INVALID_ARGUMENT;
	}

	*result = *(vec->data + index);

	return NO_ERROR;
}

ERRFUNC MiniVector_#T#_set(MiniVector *vec, const size_t index, const T *element) {

	if (index >= vec->count || element == nullptr) {
		llog(LOG_WARNING, "[MINIVECTOR] Trying to set a null element or an unreachable position\n");
		return INVALID_ARGUMENT;
	}
	vec->data[index] = *element;

	return NO_ERROR;
}

ERRFUNC MiniVector_#T#_append(MiniVector *vec, const T *element) {
	if (vec->count == vec->capacity / sizeof(T)) {
		auto err = MiniVector_#T#_grow(vec);
		RET_IF_ERR(err)
	}

	vec->data[vec->count] = *element;

	++(vec->count);

	return NO_ERROR;
}

ERRFUNC MiniVector_#T#_insert(MiniVector *vec, const size_t index, const T *element) {
	if (index > vec->count) {
		llog(LOG_WARNING, "[MINIVECTOR] %ld > %ld Unreachable position\n", index, vec->count);
		return INVALID_ARGUMENT;
	}

	if (index == vec->count) {
		auto err = MiniVector_#T#_append(vec, element);
		RET_IF_ERR(err)
	}

	if (vec->count == vec->capacity / sizeof(T)) {
		// i have to grow
		// I am doing double work here (in case realloc cannot extend the given pointer)
		// either realloc copies everything and then I move part of the array further OR
		// I move part of the array and then realloc copies everything
		// If I were to malloc new memory I would not incur in a double copy BUT i would miss out on a potential easy realloc
		// So the decision lies on the distribution of good realloc vs bad realloc
		// but I don't have the data to know this. So I'll just go with grow (realloc)
		auto err = MiniVector_#T#_grow(vec);
		RET_IF_ERR(err)
	}

	// god bless memmove
	memmove(&vec->data[index + 1], &vec->data[index], (vec->count - index) * sizeof(T));

	++(vec->count);

	// finally write the data
	auto err = MiniVector_#T#_set(vec, index, element);
	RET_IF_ERR(err)

	return NO_ERROR;
}


ERRFUNC MiniVector_#T#_remove(MiniVector *vec, const size_t index) {

	// we cant just overwrite the position to erase when
	// index is out of bound, > count or
	// index is on the last index (since we would run the risk of copying garbage)
	if (index >= vec->count) {
		llog(LOG_WARNING, "[MINIVECTOR] %ld >= %ld Unreachable position\n", index, vec->count);
		return INVALID_ARGUMENT;
	}

	if (index < vec->count - 1) {
		// just copy over it, no need to do anything else really
		memmove(&vec->data[index], &vec->data[index + 1], (vec->count - index - 1) * sizeof(T));
	}

	--vec->count;

	return NO_ERROR;
}

#undef MiniVector
