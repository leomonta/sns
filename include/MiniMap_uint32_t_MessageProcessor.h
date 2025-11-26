#pragma once

// template <K, V, I>

#include "MiniVector_uint32_t.h"
#include "MiniVector_MessageProcessor.h"



#define MiniMap MiniMap_uint32_t_MessageProcessor

typedef struct {
	MiniVector_uint32_t keys;
	MiniVector_MessageProcessor values;
} MiniMap;

/**
 * Makes a MiniMap with a preallocated array of initial_count length
 *
 * @param `initial_count` how many elements to preallocate, defaults to 10 if zero is specified
 *
 * @return a built MiniMap
 */
MiniMap MiniMap_uint32_t_MessageProcessor_make(const size_t initial_count);

/**
 * Frees up all the resources allocated by the miniMap
 *
 * @param `map` the MiniMap to destroy
 */
void MiniMap_uint32_t_MessageProcessor_destroy(MiniMap *map);

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
bool MiniMap_uint32_t_MessageProcessor_replace(MiniMap *map, const uint32_t *key, const MessageProcessor *value, bool (*eq)(const uint32_t *, const uint32_t *));

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
bool MiniMap_uint32_t_MessageProcessor_get(const MiniMap *map, const uint32_t *key, bool (*eq)(const uint32_t *, const uint32_t *), MessageProcessor* result);

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
void MiniMap_uint32_t_MessageProcessor_set(MiniMap *map, const uint32_t *key, const MessageProcessor *value, bool (*eq)(const uint32_t *, const uint32_t *));

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
bool MiniMap_uint32_t_MessageProcessor_remove(MiniMap *map, const uint32_t *key, bool (*eq)(const uint32_t *, const uint32_t *));

#undef MiniMap
