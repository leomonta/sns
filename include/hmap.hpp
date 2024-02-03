#pragma once

#include "miniVector.hpp"

namespace hmap {

	template <typename T, typename U>
	struct hmap {

		miniVector::miniVector<T> keys;
		miniVector::miniVector<U> values;
	};

	/**
	 * insert a value into the hash map trhough its relative key
	 *
	 * @param map the hashmap to insert the values into
	 * @param key the key relative for the value, yhe key is applied directly, no hash function is applied, thus the hashing step should be done before
	 * @param value
	 */
	template <typename T, typename U>
	void insert(hmap<T, U> map, T key, U element);

	/**
	 * get the value corresponding to the giving key
	 *
	 * @param map the map to get the value from
	 * @param key
	 *
	 * @return a pointer the value associated with thw given key, or nullptr if the key is not present in the map
	 */
	template <typename T, typename U>
	U get(hmap<T, U> map, T key);
} // namespace hmap
