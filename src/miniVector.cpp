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
void miniVector::destroyMiniVector(miniVector<T> *vec) {
	free(vec->data);

	// zero everythin
	vec->data = nullptr;
	vec->capacity = 0;
	vec->count = 0;
}

template <typename T>
void miniVector::grow(miniVector<T> *vec) {
	vec->data = realloc(vec->data, vec->capacity * 2);
	vec->capacity *= 2;
	// for why 2 and not 1.6 or 1.5
	// See video -> https://www.youtube.com/watch?v=GZPqDvG615k
	// essentially after 3 array being used in the same memory space, 2 performs sligthly better than 1.5 abd others
}

template <typename T>
T *miniVector::retrieve(const miniVector<T> *vec, const size_t index) {
	if (index >= vec->count) {
		// invalid pos
		return nullptr;
	}

	return vec->data[index];
}

template <typename T>
void miniVector::set(const miniVector<T> *vec, const size_t index, const T *element) {
	if (index < vec->count) {
		memcpy(vec[index], element, 1 * sizeof(T));
	}
}

template <typename T>
void miniVector::append(miniVector<T> *vec, T *element) {
	if (vec->count == vec->capacity / sizeof(T)) {
		grow(vec);
	}

	memcpy(vec->data + vec->count, element, sizeof(T));

	++(vec->count);
}

template <typename T>
void miniVector::insert(const miniVector<T> *vec, const size_t index, const T *element) {
	if (index >= vec->count) {
		// invalid position
		return;
	}

	if (vec->count == vec->capacity / sizeof(T)) {
		// i have to grow
		// I am doing double work here (in case realloc cannot extend the given pointer)
		// either realloc copies everything and then I move part of the array further OR
		// I move part of the array and then realloc copies everything
		// If I were to malloc new memory I would not incur in a double copy BUT i would miss out on a potential easy realloc
		// So the decision lies on the distribution of good realloc vs bad realloc
		// but I don't have the data to do this. So I'll just go with grow (realloc)
		grow(vec);
	}

	// god bless memmove
	memcpy(vec->data + index, vec->data + index + 1, vec->count - index);

	// finally write the data
	set(vec, index, element);

}

template <typename T>
void miniVector::remove(miniVector<T> *vec, const size_t index) {

	// we cant just overwrite the position to erase when
	// index is out of bound, > count or
	// index is on the last index (since we would run the risk of copying garbage)
	if (index >= vec->count) {
		// nothing to delete
		return;
	}

	if (index < vec->count - 1) {
		// just copy over it, no need to do anything else really
		memcpy(vec[index], vec[index + 1], (vec->count - index) * sizeof(T));
	} 

	--vec->count;
}
