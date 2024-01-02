#pragma once

#include <stddef.h>

namespace miniVector {
	template <typename T>
	struct miniVector {

		T     *data;     // data ptr
		size_t capacity; // total allocated byte
		size_t count;    // how many elements are stored at the moment
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
	 * append an element of the previously specified size at the end of the vector
	 * the element is bytecopied in the internal array
	 *
	 * @param vec the miniVecor where to append the data
	 * @param element a pointer to the data to be appended
	 */
	template <typename T>
	void append(miniVector<T> *vec, T *element);

	/**
	 * Doubles the capacity of the given vector
	 *
	 * @param vec the vector to grow
	 */
	template <typename T>
	void grow(miniVector<T> *vec);

	/**
	 * Frees all the resource allocated by vec
	 *
	 * @param vec the vector the destroy
	 */
	template <typename T>
	void destroyMiniVector(miniVector<T> *vec);

	/**
	 * Return the element at the specified position
	 *
	 * @param vec the vector to get the element from
	 * @param index the position the element should be
	 *
	 * @return the pointer to the element at pos index
	 */
	template <typename T>
	T *retrive(const miniVector<T> *vec, const size_t index);
} // namespace miniVector
