#pragma once

#include <stddef.h>
#include <stdlib.h>

// 

#include <stdint.h>

#define MiniVector MiniVector_uint32_t

typedef struct {
	uint32_t     *data;     // data ptr
	size_t capacity; // total allocated bytes
	size_t count;    // how many elements are stored at the moment / the first index that can be used
} MiniVector;

/**
 * Makes a MiniVector with a preallocated array of initial_count length
 *
 * @param[in] `initial_count` how many elements to preallocate, defaults to 10 if zero is specified
 *
 * @return a built MiniVector
 */
MiniVector MiniVector_uint32_t_make(const size_t initial_count);

/**
 * Frees all the resource allocated by vec
 *
 * @param[in] `vec` the MiniVector the destroy
 */
void MiniVector_uint32_t_destroy(MiniVector *vec);

/**
 * Doubles the capacity of the given vector
 *
 * @param[in] `vec` the MiniVector to grow
 */
void MiniVector_uint32_t_grow(MiniVector *vec);

/**
 * Return the element at the specified position
 *
 * @param[in] `vec` the MiniVector to get the element from
 * @param[in] `index` the position the element should be
 * @param[out] `result` where to place the value at the given position
 *
 * @return true if the index is in range and there is value at that position
 */
bool MiniVector_uint32_t_get(const MiniVector *vec, const size_t index, uint32_t *result);

/**
 * Set the element at index to the element given
 *
 * @param[in] `vec` the MiniVector to work on
 * @param[in] `index` the index to replace the element at, if the index is out of bounds no operation is performed
 * @param[in] `element` the element that will replace the one already at that position. Must not be nullptr
 */
void MiniVector_uint32_t_set(MiniVector *vec, const size_t index, const uint32_t *element);

/**
 * Append an element at the end of the MiniVector
 * the element is bytecopied in the internal array
 *
 * @param[in] `vec` the MiniVector where to append the data
 * @param[in] `element` a pointer to the data to be appended
 */
void MiniVector_uint32_t_append(MiniVector *vec, const uint32_t *element);

/**
 * Insert the given value at the given index, shifting (and eventaully growing) the rest of the vector
 *
 * @param[in] `vec` the MiniVector to work on
 * @param[in] `index` where to insert the given value
 * @param[in] `element` the element to insert
 */
void MiniVector_uint32_t_insert(MiniVector *vec, const size_t index, const uint32_t *element);

/**
 * Renove the element at the given index and moves every element after to keep the data contiguous
 *
 * @param[in] `vec` the MiniVector to remove an element from
 * @param[in] `index` the index of the element to be removed, if it is out of bounds no operation is performed
 */
void MiniVector_uint32_t_remove(MiniVector *vec, const size_t index);

#undef MiniVector
