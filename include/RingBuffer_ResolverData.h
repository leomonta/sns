#pragma once

#include <stddef.h>
#include <stdlib.h>

#include "errcode.h"

//

#include "ResolverData.h"

#define RingBuffer RingBuffer_ResolverData

typedef struct {
	ResolverData     *data;   // data ptr
	size_t index;  // the index we can read data from, always clamped between [0, count[
	size_t stored; // how many elements are stored at the moment
	size_t count;  // how many elements can be stored at the moment
} RingBuffer;

/**
 * Makes a RingBuffer with `initial_count` elements pre-allocated
 *
 * @param[in] `initial_count` how many elements to preallocate, defaults to 10 if zero is specified
 * @param[out] `rngb` where to place the constructed ring buffer
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC RingBuffer_ResolverData_make(const size_t initial_count, RingBuffer *rngb);

/**
 * Frees all the resource allocated by rngb
 *
 * @param[in] `rngb` the RingBuffer the destroy
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC RingBuffer_ResolverData_destroy(RingBuffer *rngb);

/**
 * Doubles the capacity of the given buffer
 *
 * @param[in] `rngb` the RingBuffer to grow
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC RingBuffer_ResolverData_grow(RingBuffer *rngb);

/**
 * Return the next available element and removes it from the ring buffer
 *
 * @param[in] `rngb` the RingBuffer to get the element from
 * @param[out] `result` where to place the retrieved data.
 * if NO_OP or another error is returned this values should be ignored as there was no data to communicate back
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC RingBuffer_ResolverData_retrieve(RingBuffer *rngb, ResolverData *result);

/**
 * Append an element at the end of the already stored data
 * the element is bytecopied in the internal array
 *
 * @param[in] `rngb` the RingBuffer where to append the data
 * @param[in] `element` a pointer to the data to be appended
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC RingBuffer_ResolverData_append(RingBuffer *rngb, const ResolverData *element);

/**
 * Return the next available element without removing it from the ring buffer
 *
 * @param[in] `rngb` the RingBuffer to get the element from
 * @param[out] `result` where to place the retrieved data.
 * if NO_OP or another error returned this values should be ignored as there was no data to communicate back
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC RingBuffer_ResolverData_peek(const RingBuffer *rngb, ResolverData *result);
