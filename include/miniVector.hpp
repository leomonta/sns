#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstring>

namespace miniVector {
	template <typename T>
	struct MiniVector {
		T     *data;     // data ptr
		size_t capacity; // total allocated bytes
		size_t count;    // how many elements are stored at the moment / the first index that can be used
	};

	/**
	 * Makes a MiniVector with a preallocated array of initial_count length
	 *
	 * @param `initial_count` how many elements to preallocate, defaults to 10 if zero is specified
	 *
	 * @return a built MiniVector
	 */
	template <typename T>
	MiniVector<T> make(const size_t initial_count) {
		auto capacity = initial_count == 0 ? 10 : initial_count;

		MiniVector<T> res = {
		    .data     = static_cast<T *>(malloc(capacity * sizeof(T))),
		    .capacity = capacity * sizeof(T),
		    .count    = 0,
		};

		// copy elision
		return res;
	}

	/**
	 * Frees all the resource allocated by vec
	 *
	 * @param `vec` the MiniVector the destroy
	 */
	template <typename T>
	void destroy(MiniVector<T> *vec) {

		free(vec->data);

		// zero everythin
		vec->data     = nullptr;
		vec->capacity = 0;
		vec->count    = 0;
	}

	/**
	 * Doubles the capacity of the given vector
	 *
	 * @param `vec` the MiniVector to grow
	 */
	template <typename T>
	void grow(MiniVector<T> *vec) {

		vec->data = static_cast<T *>(realloc(vec->data, vec->capacity * 2));
		vec->capacity *= 2;
		// for why 2 and not 1.6 or 1.5
		// See video -> https://www.youtube.com/watch?v=GZPqDvG615k
		// essentially after 3 array being used in the same memory space, 2 performs sligthly better than 1.5 abd others
	}

	/**
	 * Return the element at the specified position
	 *
	 * @param `vec` the MiniVector to get the element from
	 * @param `index` the position the element should be
	 *
	 * @return the pointer to the element at pos index
	 */
	template <typename T>
	T *get(const MiniVector<T> *vec, const size_t index) {

		if (index >= vec->count) {
			// invalid pos
			return nullptr;
		}

		return vec->data + index;
	}

	/**
	 * Set the element at index to the element given
	 *
	 * @param `vec` the MiniVector to work on
	 * @param `index` the index to replace the element at, if the index is out of bounds no operation is performed
	 * @param `element` the element to replace that will replace the one already in the vectori. Must not be nullptr
	 */
	template <typename T>
	void set(MiniVector<T> *vec, const size_t index, const T *element) {

		if (index < vec->count) {
			memcpy(vec->data + index, element, sizeof(T));
		}
	}

	/**
	 * Append an element at the end of the MiniVector
	 * the element is bytecopied in the internal array
	 *
	 * @param `vec` the MiniVector where to append the data
	 * @param `element` a pointer to the data to be appended
	 */
	template <typename T>
	void append(MiniVector<T> *vec, const T *element) {
		if (vec->count == vec->capacity / sizeof(T)) {
			grow(vec);
		}

		memcpy(vec->data + vec->count, element, sizeof(T));

		++(vec->count);
	}

	/**
	 * Insert the given value at the given index, shofting (and eventaully growing) the rest of the vector
	 *
	 * @param `vec` the MiniVector to work on
	 * @param `index` where to insert the given value
	 * @param `element` the element to insert
	 *
	 */
	template <typename T>
	void insert(MiniVector<T> *vec, const size_t index, const T *element) {
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
			// but I don't have the data to know this. So I'll just go with grow (realloc)
			grow(vec);
		}

		// god bless memmove
		memmove(vec->data + index + 1, vec->data + index, vec->count - index);

		// finally write the data
		set(vec, index, element);
	}

	/**
	 * Renove the element at the given index and moves every element after to keep the data contiguous
	 *
	 * @param `vec` the MiniVector to remove an element from
	 * @param `index` the index of the element to be removed, if it is out of bounds no operation is performed
	 */
	template <typename T>
	void remove(MiniVector<T> *vec, const size_t index) {

		// we cant just overwrite the position to erase when
		// index is out of bound, > count or
		// index is on the last index (since we would run the risk of copying garbage)
		if (index >= vec->count) {
			// nothing to delete
			return;
		}

		if (index < vec->count - 1) {
			// just copy over it, no need to do anything else really
			memcpy(vec->data + index, vec->data + index + 1, (vec->count - index - 1) * sizeof(T));
		}

		--vec->count;
	}
} // namespace miniVector
