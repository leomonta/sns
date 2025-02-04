#pragma once

#include "miniVector.hpp"

namespace miniMap {

	template <typename K, typename V>
	struct miniMap {

		miniVector::miniVector<K> keys;
		miniVector::miniVector<V> values;
	};

	/**
	 * replace the value at the given key with the given value and returns true
	 * if the key is not found no operation is performed and returns false
	 *
	 * @param map the hashmap to insert the values into
	 * @param key the key relative for the value, the key is applied directly, no hash function is applied, thus the hashing step should be done before
	 * @param value the value to associate to the given key
	 * @return true if the value at key was replaced
	 */
	template <typename K, typename V>
	bool replace(miniMap<K, V> *map, K *key, V *value);

	/**
	 * get the value corresponding to the giving key
	 *
	 * @param map the map to get the value from
	 * @param key the key of the value to return
	 *
	 * @return a pointer the value associated with the given key, or nullptr if the key is not present in the map
	 */
	template <typename K, typename V>
	V get(miniMap<K, V> *map, K *key);

	/**
	 * insert a value into the hash map trhough its relative key
	 * if the key is present replaces the stored value with the given one
	 * else appends both key and value
	 *
	 * @param map the hashmap to insert the values into
	 * @param key the key relative for the value, the key is applied directly, no hash function is applied, thus the hashing step should be done before
	 * @param value the value to associate to the given key
	 */
	template <typename K, typename V>
	void set(miniMap<K, V> *map, K *key, V *value);

	/**
	 * Attempts to remove the key and its relative value
	 * return true if succeedes
	 *
	 * @param map the hashmap to insert the values into
	 * @param key the key relative for the value, yhe key is applied directly, no hash function is applied, thus the hashing step should be done before
	 * @return if the key, value pair has been removed 
	 */
	template <typename K, typename V>
	bool remove(miniMap<K, V> *map, K *key);

} // namespace miniMap
