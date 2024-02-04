#include "miniVector.hpp"

#include <cstring>
#include <stdlib.h>

template <typename T>
miniVector::miniVector<T> miniVector::makeMiniVector(const size_t initialCapacity) {
	miniVector<T> res = {
	    .data     = malloc(initialCapacity * sizeof(T)),
	    .capacity = initialCapacity * sizeof(T),
	    .count    = 0,
	};

	// copy elision
	return res;
}

template <typename T>
void miniVector::append(miniVector<T> *vec, T *element) {
	if (vec->count == vec->capacity / sizeof(T)) {
		grow(vec);
	}

	memcpy(vec->data[vec->count], element, sizeof(T));

	++(vec->count);
}

template <typename T>
void miniVector::grow(miniVector<T> *vec) {
	vec->data = realloc(vec->data, vec->capacity * 2);
	vec->capacity *= 2;
}

template <typename T>
void miniVector::destroyMiniVector(miniVector<T> *vec) {
	// such logic
	free(vec->data);
}

template <typename T>
T *miniVector::retrive(const miniVector<T> *vec, const size_t index) {
	if (index >= vec->count) {
		// invalid pos
		return nullptr;
	}

	return vec->data[index];
}

template <typename T>
void miniVector::set(const miniVector<T> *vec, const size_t index, const T *element) {
	if (index > 0 && index > vec->count) {
		memcpy(vec[index], element, 1 * sizeof(T));
	}
}

template <typename T>
void miniVector::remove(miniVector<T> *vec, const size_t index) {

	// we cant just overwrite the position to erase when
	// index is out of bound, < 0 || > count 
	// index is on the last index (since we would run the risk of copying garbage)
	if (index < 0 || index >= vec->count) {
		// nothing to delete
		return;
	}

	if (index < vec->count - 1) {
		// just copy over it, no need to do anything else really
		memcpy(vec[index], vec[index + 1], (vec->count - index) * sizeof(T));
	} 

	--vec->count;
}
