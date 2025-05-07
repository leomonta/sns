#pragma once

#include "miniVector.hpp"

namespace miniMap {

	template <typename K, typename V>
	struct MiniMap {

		miniVector::MiniVector<K> keys;
		miniVector::MiniVector<V> values;
	};

	/**
	 * Makes a MiniMap with a preallocated array of initial_count length
	 *
	 * @param `initial_count` how many elements to preallocate, defaults to 10 if zero is specified
	 *
	 * @return a built MiniMap
	 */
	template <typename K, typename V>
	MiniMap<K, V> make(const size_t initial_count) {

		MiniMap<K, V> res;

		res.keys = miniVector::make<K>(initial_count);
		res.values = miniVector::make<V>(initial_count);

		return res;
	}

	/**
	 * Frees up all the resources allocated by the miniMap
	 *
	 * @param `map` the MiniMap to destroy
	 */
	template <typename K, typename V>
	void destroy(MiniMap<K, V> *map) {

		miniVector::destroy(&map->keys);
		miniVector::destroy(&map->values);
	}

	/**
	 * Replace the value at the given key with the given value and returns true.
	 * If the key is not found no operation is performed and returns false
	 *
	 * @param `map` the MiniMap to insert the values into
	 * @param `key` the key relative for the value, the key is applied directly, no hash function is applied, thus the hashing step should be done before
	 * @param `value` the value to associate to the given key
	 *
	 * @return true if the value at key was replaced
	 */
	template <typename K, typename V>
	bool replace(MiniMap<K, V> *map, const K *key, const V *value) {

		// find it
		for (size_t i = 0; i < map->keys.count; ++i) {

			// if found
			if (map->keys.data[i] == *key) {

				// replace it
				miniVector::set(&map->values, i, value);
				return true;
			}
		}

		return false;
	}

	/**
	 * Replace the value at the given key with the given value and returns true.
	 * If the key is not found no operation is performed and returns false
	 * Check the equality of the key with the given equality function
	 *
	 * @param `map` the MiniMap to insert the values into
	 * @param `key` the key relative for the value, the key is applied directly, no hash function is applied, thus the hashing step should be done before
	 * @param `value` the value to associate to the given key
	 * @param `eq` the function that decides if the keys are equal
	 *
	 * @return true if the value at key was replaced
	 */
	template <typename K, typename V>
	bool replace_eq(MiniMap<K, V> *map, const K *key, const V *value, bool (*eq)(const K *, const K *)) {

		// find it
		for (size_t i = 0; i < map->keys.count; ++i) {

			// if found
			if (eq(map->keys.data + i, key)) {

				// replace it
				miniVector::set(&map->values, i, value);
				return true;
			}
		}

		return false;
	}

	/**
	 * Get the value corresponding to the giving key
	 *
	 * @param `map` the map to get the value from
	 * @param `key` the key of the value to return
	 *
	 * @return a pointer the value associated with the given key, or nullptr if the key is not present in the map
	 */
	template <typename K, typename V>
	V* get(const MiniMap<K, V> *map, const K *key) {

		// find it
		for (size_t i = 0; i < map->keys.count; ++i) {

			// if found
			if (map->keys.data[i] == *key) {

				return map->values.data + i;
			}
		}

		return nullptr;
	}

	/**
	 * Get the value corresponding to the giving key
	 * Check the equality of the key with the given equality function
	 *
	 * @param `map` the map to get the value from
	 * @param `key` the key of the value to return
	 * @param `eq` the function that decides if the keys are equal
	 `*
	 * @return a pointer the value associated with the given key, or nullptr if the key is not present in the map
	 */
	template <typename K, typename V>
	V* get_eq(const MiniMap<K, V> *map, const K *key, bool (*eq)(const K *, const K *)) {

		// find it
		for (size_t i = 0; i < map->keys.count; ++i) {

			// if found
			if (eq(map->keys.data + i, key)) {

				return map->values.data + i;
			}
		}

		return nullptr;
	}

	/**
	 * Insert a value into the hash map trhough its relative key
	 * if the key is present replaces the stored value with the given one
	 * else appends both key and value
	 *
	 * @param `map` the MiniMap to set the values into
	 * @param `key` the key relative for the value, the key is applied directly, no hash function is applied, thus the hashing step should be done before
	 * @param `value` the value to associate to the given key
	 */
	template <typename K, typename V>
	void set(MiniMap<K, V> *map, const K *key, const V *value) {

		// find it
		for (size_t i = 0; i < map->keys.count; ++i) {

			// if found
			if (map->keys.data[i] == *key) {

				// replace it
				miniVector::set(&map->values, i, value);
				return;
			}
		}

		// else append
		miniVector::append(&map->keys, key);
		miniVector::append(&map->values, value);
	}

	/**
	 * Insert a value into the hash map trhough its relative key
	 * if the key is present replaces the stored value with the given one
	 * else appends both key and value
	 * Check the equality of the key with the given equality function
	 *
	 * @param `map` the MiniMap to set the values into
	 * @param `key` the key relative for the value, the key is applied directly, no hash function is applied, thus the hashing step should be done before
	 * @param `value` the value to associate to the given key
	 * @param `eq` the function that decides if the keys are equal
	 */
	template <typename K, typename V>
	void set_eq(MiniMap<K, V> *map, const K *key, const V *value, bool (*eq)(const K *, const K *)) {

		// find it
		for (size_t i = 0; i < map->keys.count; ++i) {

			// if found
			if (eq(map->keys.data + i, key)) {

				// replace it
				miniVector::set(&map->values, i, value);
				return;
			}
		}

		// else append
		miniVector::append(&map->keys, key);
		miniVector::append(&map->values, value);
	}

	/**
	 * Attempts to remove the key and its relative value
	 *
	 * @param `map` the MiniMap to remove the values from
	 * @param `key` the key relative for the value, yhe key is applied directly, no hash function is applied, thus the hashing step should be done before
	 *
	 * @return true if the key value pair has been removed
	 */
	template <typename K, typename V>
	bool remove(MiniMap<K, V> *map, const K *key) {

		for (size_t i = 0; i < map->keys.count; ++i) {

			// if found
			if (map->keys.data[i] == *key) {

				miniVector::remove(&map->keys, i);
				miniVector::remove(&map->values, i);

				return true;
			}
		}

		return false;
	}

	/**
	 * Attempts to remove the key and its relative value
	 * return true if succeedes
	 * Check the equality of the key with the given equality function
	 *
	 * @param `map` the MiniMap to remove the values from
	 * @param `key` the key relative for the value, yhe key is applied directly, no hash function is applied, thus the hashing step should be done before
	 * @param `eq` the function that decides if the keys are equal
	 *
	 * @return if the key value pair has been removed
	 */
	template <typename K, typename V>
	bool remove_eq(MiniMap<K, V> *map, const K *key, bool (*eq)(const K *, const K *)) {

		for (size_t i = 0; i < map->keys.count; ++i) {

			// if found
			if (eq(map->keys.data + i, key)) {

				miniVector::remove(&map->keys, i);
				miniVector::remove(&map->values, i);

				return true;
			}
		}

		return false;
	}

} // namespace miniMap
