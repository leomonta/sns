#pragma once

#include <stddef.h>
#include <stdlib.h>

#include "errcode.h"

// template <T, I>

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
 * @param[in] `initial_count` how many elements to preallocate, defaults to 10 if zero is specified
 * @param[out] `vec` where to place the constructed MiniVector 
 *
 * @return an error code describing if and which error occurred
 *
 * - `FAILED_ALLOCATION`: the MiniVector could not be allocated. the structure was not constructed and should not be used nor destroyed
 */
ERRFUNC MiniVector_#T#_make(const size_t initial_count, MiniVector *vec);

/**
 * Frees all the resource allocated by vec
 *
 * @param[in] `vec` the MiniVector the destroy
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC MiniVector_#T#_destroy(MiniVector *vec);

/**
 * Doubles the capacity of the given vector
 *
 * @param[in] `vec` the MiniVector to grow
 * 
 * @return an error code describing if and which error occurred
 *
 * - `FAILED_ALLOCATION`: the vector was not grown but it retains the old state
 */
ERRFUNC MiniVector_#T#_grow(MiniVector *vec);

/**
 * Return the element at the specified position
 *
 * @param[in] `vec` the MiniVector to get the element from
 * @param[in] `index` the position the element should be
 * @param[out] `result` where to place the value at the given position, 
 * if `NO_OP` ot another error is returned this values should be ignored as there was no data to communicate back
 *
 * @return an error code describing if and which error occurred
 *
 * - `INVALID_ARGUMENT`: the index given exceeds the amount of elements available in the structure. No data was communicated back
 */
ERRFUNC MiniVector_#T#_get(const MiniVector *vec, const size_t index, T *result);

/**
 * Set the element at index to the element given
 *
 * @param[in] `vec` the MiniVector to work on
 * @param[in] `index` the index to replace the element at, if the index is out of bounds no operation is performed
 * @param[in] `element` the element that will replace the one already at that position. Must not be nullptr
 *
 * @return an error code describing if and which error occurred
 * 
 * - `INVALID_ARGUMENT`: the index given exceeds the amount of elements available in the structure or the given element is `nullptr`. No data is set
 */
ERRFUNC MiniVector_#T#_set(MiniVector *vec, const size_t index, const T *element);

/**
 * Append an element at the end of the MiniVector
 * the element is bytecopied in the internal array
 *
 * @param[in] `vec` the MiniVector where to append the data
 * @param[in] `element` a pointer to the data to be appended
 *
 * @return an error code describing if and which error occurred
 *
 * - `FAILED_ALLOCATION`: the vector had to grown but it wasn't possible. No data was added
 * - `INVALID_ARGUMENT`: the given element is `nullptr`. No data was added
 */
ERRFUNC MiniVector_#T#_append(MiniVector *vec, const T *element);

/**
 * Insert the given value at the given index, shifting (and eventaully growing) the rest of the vector
 *
 * If index == count the item will be appended at the end
 *
 * @param[in] `vec` the MiniVector to work on
 * @param[in] `index` where to insert the given value
 * @param[in] `element` the element to insert
 *
 * @return an error code describing if and which error occurred
 *
 * - `FAILED_ALLOCATION`: the vector had to grown but it wasn't possible. No data was added
 * - `INVALID_ARGUMENT`: the index given exceeds the amount of elements available in the structure or the given element is `nullptr`. No data was added
 */
ERRFUNC MiniVector_#T#_insert(MiniVector *vec, const size_t index, const T *element);

/**
 * Renove the element at the given index and moves every element after to keep the data contiguous
 *
 * @param[in] `vec` the MiniVector to remove an element from
 * @param[in] `index` the index of the element to be removed, if it is out of bounds no operation is performed
 *
 * @return an error code describing if and which error occurred
 *
 * - `INVALID_ARGUMENT`: the index given exceeds the amount of elements available in the structure. No data was removed
 */
ERRFUNC MiniVector_#T#_remove(MiniVector *vec, const size_t index);

#undef MiniVector
