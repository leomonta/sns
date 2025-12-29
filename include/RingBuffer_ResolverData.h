#pragma once

#include <stddef.h>
#include <stdlib.h>

//

#include "methods.h"

#define RingBuffer RingBuffer_ResolverData

typedef struct {
	ResolverData     *data;   // data ptr
	size_t index;  // the index we can read data from, always clamped between [0, count[
	size_t stored; // how many elements are stored at the moment
	size_t count;  // how many elements can be stored at the moment
} RingBuffer;

/**
 * Makes a RingBuffer with a preallocated array of initial_count lenght
 *
 * @param `initial_count` how many elements to preallocate, defaults to 10 if zero is specified
 *
 * @return a built RingBuffer
 */
RingBuffer RingBuffer_ResolverData_make(const size_t initial_count);

/**
 * Frees all the resource allocated by rngb
 *
 * @param `rngb` the RingBuffer the destroy
 */
void RingBuffer_ResolverData_destroy(RingBuffer *rngb);

/**
 * Doubles the capacity of the given buffer
 *
 * @param `rngb` the RingBuffer to grow
 */
void RingBuffer_ResolverData_grow(RingBuffer *rngb);

/**
 * Return the next available element and removes it from the ring buffer
 *
 * @param `rngb` the RingBuffer to get the element from
 *
 * @return the pointer to the next available element
 */
bool RingBuffer_ResolverData_retrieve(RingBuffer *rngb, ResolverData *result);

/**
 * Append an element at the end of the already stored data
 * the element is bytecopied in the internal array
 *
 * @param `rngb` the RingBuffer where to append the data
 * @param `element` a pointer to the data to be appended
 */
void RingBuffer_ResolverData_append(RingBuffer *rngb, const ResolverData *element);

/**
 * Return the next available element without removing it from the ring buffer
 *
 * @param `rngb` the RingBuffer to get the element from
 *
 * @return the pointer to the next available element
 */
bool RingBuffer_ResolverData_peek(const RingBuffer *rngb, ResolverData* result);
