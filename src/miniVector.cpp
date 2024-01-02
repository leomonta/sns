#include "miniVector.hpp"

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
