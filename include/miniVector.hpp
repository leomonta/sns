#pragma once

#include <stddef.h>

namespace miniVector {
	template <typename T>
	struct miniVector {
		T     *data;     // data ptr
		size_t capacity; // total allocated bytes
		size_t count;    // how many elements are stored at the moment / the first index that can be used
	};

	/**
	 * Make a mini vector with a preallocated array of elementSize * initalCount lenght
	 *
	 * @param initialCapacity how many bytes to preallocate;
	 *
	 * @return a built miniVector
	 */
	template <typename T>
	miniVector<T> makeMiniVector(const size_t initialCapacity);

	/**
	 * Frees all the resource allocated by vec
	 *
	 * @param vec the vector the destroy
	 */
	template <typename T>
	void destroyMiniVector(miniVector<T> *vec);

	/**
	 * Doubles the capacity of the given vector
	 *
	 * @param vec the vector to grow
	 */
	template <typename T>
	void grow(miniVector<T> *vec);

	/**
	 * Return the element at the specified position
	 *
	 * @param vec the vector to get the element from
	 * @param index the position the element should be
	 *
	 * @return the pointer to the element at pos index
	 */
	template <typename T>
	T *get(const miniVector<T> *vec, const size_t index);

	/**
	 * Set the element at index to the element given
	 *
	 * @param vec the vector to work on
	 * @param index the index to replace the element at, if the index is out of bounds no operation is performed
	 * @param element the element to replace that will replace the one already in the vectori
	 */
	template <typename T>
	void set(const miniVector<T> *vec, const size_t index, const T *element);

	/**
	 * append an element of the previously specified size at the end of the vector
	 * the element is bytecopied in the internal array
	 *
	 * @param vec the miniVecor where to append the data
	 * @param element a pointer to the data to be appended
	 */
	template <typename T>
	void append(miniVector<T> *vec, T *element);

	/**
	 * Insert the given value at the given index, shofting (and eventaully growing) the rest of the vector
	 *
	 * @param vec the vector to work on
	 * @param index where to insert the given value
	 * @param element the element to insert
	 *
	 */
	template <typename T>
	void insert(const miniVector<T> *vec, const size_t index, const T *element);

	/**
	 * Renove the elemente at the given index and moves every element after to keep the data contiguous
	 *
	 * @param vec the miniVector to remove an element from
	 * @param index the index of the element to be removed, if it is out of bounds no operation is performed
	 */
	template <typename T>
	void remove(miniVector<T> *vec, const size_t index);
} // namespace miniVector
