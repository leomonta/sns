//

#include "RingBuffer_ResolverData.h"

#include "logger.h"
#include "utils.h"

#include <errno.h>

#define RingBuffer RingBuffer_ResolverData

#define GROW_RATE 2

RingBuffer RingBuffer_ResolverData_make(const size_t initial_count) {
	auto capacity = initial_count == 0 ? 10 : initial_count;

	RingBuffer res = {
	    .data   = malloc(capacity * sizeof(ResolverData)),
	    .index  = 0,
	    .stored = 0,
	    .count  = capacity,
	};
	TEST_ALLOC(res.data)

	// copy elision
	return res;
}

void RingBuffer_ResolverData_destroy(RingBuffer *rngb) {

	free(rngb->data);

	// zero everything
	rngb->data   = nullptr;
	rngb->index  = 0;
	rngb->stored = 0;
	rngb->count  = 0;
}

void RingBuffer_ResolverData_grow(RingBuffer *rngb) {

	rngb->data = realloc(rngb->data, rngb->count * GROW_RATE * sizeof(ResolverData));
	rngb->count *= GROW_RATE;
	TEST_ALLOC(rngb->data)
	// for why 2 and not 1.6 or 1.5
	// See video -> https://www.youtube.com/watch?v=GZPqDvG615k
	// essentially after 3 array being used in the same memory space, 2 performs sligthly better than 1.5 and others

	// just expanding the capacity might invalidate the buffer because some data might be behind the current reading index
	// so we need to fix this
	// we can just deal with the hardest case and that deals with every other

	// with `r` being the read index `index`
	//             r
	// [ g h i j k a b c d e f _ _ _ _ _ _ _ _ _ _ _ ]
	//   |-------| <- copy this
	//
	//                 here -> |-------|
	// [ g h i j k a b c d e f g h i j k _ _ _ _ _ _ ]        step 1
	//
	//   |-------| <- and delete this
	// [ _ _ _ _ _ a b c d e f g h i j k _ _ _ _ _ _ ]        step 2

	// step 1
	// copy `index` amount of bytes from the start to the old end of the buffer (count / 2 since we have just doubled it)
	memcpy(rngb->data + rngb->count / GROW_RATE, rngb->data, rngb->index * sizeof(ResolverData));

	// tecnically this step is not needed, the index has been moved
	// but this smells of security vulnerability so...
	// step 2
	memset(rngb->data, 0, rngb->index * sizeof(ResolverData));
}

bool RingBuffer_ResolverData_retrieve(RingBuffer *rngb, ResolverData *result) {

	if (rngb->stored == 0) {
		// no data
		return false;
	}

	memcpy(result, rngb->data + rngb->index, sizeof(ResolverData));
	memset(rngb->data + rngb->index, 0, sizeof(ResolverData));

	++rngb->index;
	rngb->index = rngb->index % rngb->count;
	--rngb->stored;

	return true;
}


void RingBuffer_ResolverData_append(RingBuffer *rngb, const ResolverData *element) {
	if (rngb->stored == rngb->count) {
		RingBuffer_ResolverData_grow(rngb);
	}

	auto write_index = (rngb->index + rngb->stored) % rngb->count;

	memcpy(rngb->data + write_index, element, sizeof(ResolverData));

	++rngb->stored;
}

bool RingBuffer_ResolverData_peek(const RingBuffer *rngb, ResolverData* result) {

	if (rngb->stored == 0) {
		// no data
		return false;
	}

	memcpy(result, rngb->data + rngb->index, sizeof(ResolverData));

	return true;
}

#undef RingBuffer
