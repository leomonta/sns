#pragma once

// template <K, V, I>

#include "MiniVector_#K#.h"
#include "MiniVector_#V#.h"

I

#define MiniMap MiniMap_#K#_#V#

typedef struct {
	MiniVector_#K# keys;
	MiniVector_#V# values;
	bool (*eq_fun)(const K *, const K *);
} MiniMap;

/**
 * Makes a MiniMap with a preallocated array of initial_count length
 *
 * @param[in] `initial_count` how many elements to preallocate, defaults to 10 if zero is specified
 * @param[in] `eq` a function to compare the equality of two keys
 *
 * @return a built MiniMap
 */
MiniMap MiniMap_#K#_#V#_make(const size_t initial_count, bool (*eq)(const K *, const K *));

/**
 * Frees up all the resources allocated by the miniMap
 *
 * @param[in] `map` the MiniMap to destroy
 */
void MiniMap_#K#_#V#_destroy(MiniMap *map);

/**
 * Replace the value at the given key with the given value and returns true.
 * If the key is not found no operation is performed and returns false
 *
 * @param[in] `map` the MiniMap to insert the values into
 * @param[in] `key` the key relative for the value, the key is applied directly, no hash function is applied, thus the hashing step should be done before
 * @param[in] `value` the value to associate to the given key
 *
 * @return true if the value at key was replaced
 */
bool MiniMap_#K#_#V#_replace(MiniMap *map, const K *key, const V *value);

/**
 * Get the value corresponding to the giving key
 *
 * @param[in] `map` the map to get the value from
 * @param[in] `key` the key of the value to return
 * @param[out] `result` where to place the value associated with the given array
 *
 * @return a pointer the value associated with the given key, or nullptr if the key is not present in the map
 */
bool MiniMap_#K#_#V#_get(const MiniMap *map, const K *key, V *result);

/**
 * Get the value corresponding to the giving key
 *
 * @param[in] `map` the map to get the value from
 * @param[in] `index` the index of the entry to return
 * @param[out] `key` the key at the specified index
 * @param[out] `result` where to place the value associated with the given array
 *
 * @return a pointer the value associated with the given key, or nullptr if the key is not present in the map
 */
bool MiniMap_#K#_#V#_get_by_index(const MiniMap *map, const size_t index, K *key, V *result);

/**
 * Insert a value into the hash map trhough its relative key
 * if the key is present replaces the stored value with the given one
 * else appends both key and value
 *
 * @param[in] `map` the MiniMap to set the values into
 * @param[in] `key` the key relative for the value, the key is applied directly, no hash function is applied, thus the hashing step should be done before
 * @param[in] `value` the value to associate to the given key
 */
void MiniMap_#K#_#V#_set(MiniMap *map, const K *key, const V *value);

/**
 * Attempts to remove the key and its relative value
 * return true if succeedes
 *
 * @param[in] `map` the MiniMap to remove the values from
 * @param[in] `key` the key relative for the value, yhe key is applied directly, no hash function is applied, thus the hashing step should be done before
 *
 * @return if the key value pair has been removed
 */
bool MiniMap_#K#_#V#_remove(MiniMap *map, const K *key);

#undef MiniMap
