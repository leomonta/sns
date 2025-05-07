#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstring>

namespace ringBuffer {
	template <typename T>
	struct RingBuffer {
		T     *data;   // data ptr
		size_t index;  // the index we can read data from, always clamped between [0, count[
		size_t stored; // how many elements are stored at the moment
		size_t count;  // how many elements can be stored at the moment
	};

	/**
	 * Makes a RingBuffer with a preallocated array of initial_count lenght
	 *
	 * @param `initial_count` how many elements to preallocate, defaults to 10 if zero is specified
	 *
	 * @return a built RingBuffer
	 */
	template <typename T>
	RingBuffer<T> make(const size_t initial_count) {
		auto capacity = initial_count == 0 ? 10 : initial_count;

		RingBuffer<T> res = {
		    .data   = static_cast<T *>(malloc(capacity * sizeof(T))),
		    .index  = 0,
		    .stored = 0,
		    .count  = capacity,
		};

		// copy elision
		return res;
	}

	/**
	 * Frees all the resource allocated by rngb
	 *
	 * @param `rngb` the RingBuffer the destroy
	 */
	template <typename T>
	void destroy(RingBuffer<T> *rngb) {

		free(rngb->data);

		// zero everything
		rngb->data   = nullptr;
		rngb->index  = 0;
		rngb->stored = 0;
		rngb->count  = 0;
	}

	/**
	 * Doubles the capacity of the given buffer
	 *
	 * @param `rngb` the RingBuffer to grow
	 */
	template <typename T>
	void grow(RingBuffer<T> *rngb) {

		rngb->data = static_cast<T *>(realloc(rngb->data, rngb->count * 2 * sizeof(T)));
		rngb->count *= 2;
		// for why 2 and not 1.6 or 1.5
		// See video -> https://www.youtube.com/watch?v=GZPqDvG615k
		// essentially after 3 array being used in the same memory space, 2 performs sligthly better than 1.5 and others

		// fixing the data

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
		memcpy(rngb->data + rngb->count / 2, rngb->data, rngb->index * sizeof(T));

		// tecnically this step is not needed, the index has been moved
		// but this smells of security vulnerability so...
		// step 2
		memset(rngb->data, 0, rngb->index * sizeof(T));
	}

	/**
	 * Return the next available element and removes it from the ring buffer
	 *
	 * @param `rngb` the RingBuffer to get the element from
	 *
	 * @return the pointer to the next available element
	 */
	template <typename T>
	T *retrieve(RingBuffer<T> *rngb) {

		if (rngb->stored == 0) {
			// no data
			return nullptr;
		}

		auto res = rngb->data + rngb->index;

		++rngb->index;
		rngb->index = rngb->index % rngb->count;
		--rngb->stored;

		return res;
	}

	/**
	 * Append an element at the end of the already stored data
	 * the element is bytecopied in the internal array
	 *
	 * @param `rngb` the RingBuffer where to append the data
	 * @param `element` a pointer to the data to be appended
	 */
	template <typename T>
	void append(RingBuffer<T> *rngb, const T *element) {
		if (rngb->stored == rngb->count) {
			grow(rngb);
		}

		auto write_index = (rngb->index + rngb->stored) % rngb->count;

		memcpy(rngb->data + write_index, element, sizeof(T));

		++rngb->stored;
	}

	/**
	 * Return the next available element without removing it from the ring buffer
	 *
	 * @param `rngb` the RingBuffer to get the element from
	 *
	 * @return the pointer to the next available element
	 */
	template <typename T>
	T *peek(const RingBuffer<T> *rngb) {

		if (rngb->stored == 0) {
			// no data
			return nullptr;
		}

		return rngb->data + rngb->index;
	}

} // namespace ringBuffer
