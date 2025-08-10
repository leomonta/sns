#pragma once

#include <stddef.h>
#include <stdlib.h>

//template <T, I>

I

#define MiniVector MiniVector_#T#

typedef struct {
	T     *data;     // data ptr
	size_t capacity; // total allocated bytes
	size_t count;    // how many elements are stored at the moment / the first index that can be used
} MiniVector;

/**
 * Makes a MiniVector with a preallocated array of initial_count length
 *
 * @param `initial_count` how many elements to preallocate, defaults to 10 if zero is specified
 *
 * @return a built MiniVector
 */
MiniVector MiniVector_#T#_make(const size_t initial_count);

/**
 * Frees all the resource allocated by vec
 *
 * @param `vec` the MiniVector the destroy
 */
void MiniVector_#T#_destroy(MiniVector *vec);

/**
 * Doubles the capacity of the given vector
 *
 * @param `vec` the MiniVector to grow
 */
void MiniVector_#T#_grow(MiniVector *vec);

/**
 * Return the element at the specified position
 *
 * @param `vec` the MiniVector to get the element from
 * @param `index` the position the element should be
 *
 * @return the pointer to the element at pos index
 */
bool MiniVector_#T#_get(const MiniVector *vec, const size_t index, T* result);

/**
 * Set the element at index to the element given
 *
 * @param `vec` the MiniVector to work on
 * @param `index` the index to replace the element at, if the index is out of bounds no operation is performed
 * @param `element` the element to replace that will replace the one already in the vectori. Must not be nullptr
 */
void MiniVector_#T#_set(MiniVector *vec, const size_t index, const T *element);

/**
 * Append an element at the end of the MiniVector
 * the element is bytecopied in the internal array
 *
 * @param `vec` the MiniVector where to append the data
 * @param `element` a pointer to the data to be appended
 */
void MiniVector_#T#_append(MiniVector *vec, const T *element);

/**
 * Insert the given value at the given index, shofting (and eventaully growing) the rest of the vector
 *
 * @param `vec` the MiniVector to work on
 * @param `index` where to insert the given value
 * @param `element` the element to insert
 *
 */
void MiniVector_#T#_insert(MiniVector *vec, const size_t index, const T *element);

/**
 * Renove the element at the given index and moves every element after to keep the data contiguous
 *
 * @param `vec` the MiniVector to remove an element from
 * @param `index` the index of the element to be removed, if it is out of bounds no operation is performed
 */
void MiniVector_#T#_remove(MiniVector *vec, const size_t index);

#undef MiniVector
