#pragma once

// 

#include "MiniVector_StringRef.h"
#include "MiniVector_StringRef.h"

#include "StringRef.h"

#define MiniMap MiniMap_StringRef_StringRef

typedef struct {
	MiniVector_StringRef keys;
	MiniVector_StringRef values;
} MiniMap;

/**
 * Makes a MiniMap with a preallocated array of initial_count length
 *
 * @param `initial_count` how many elements to preallocate, defaults to 10 if zero is specified
 *
 * @return a built MiniMap
 */
MiniMap MiniMap_StringRef_StringRef_make(const size_t initial_count);

/**
 * Frees up all the resources allocated by the miniMap
 *
 * @param `map` the MiniMap to destroy
 */
void MiniMap_StringRef_StringRef_destroy(MiniMap *map);

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
bool MiniMap_StringRef_StringRef_replace(MiniMap *map, const StringRef *key, const StringRef *value, bool (*eq)(const StringRef *, const StringRef *));

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
bool MiniMap_StringRef_StringRef_get(const MiniMap *map, const StringRef *key, bool (*eq)(const StringRef *, const StringRef *), StringRef* result);

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
void MiniMap_StringRef_StringRef_set(MiniMap *map, const StringRef *key, const StringRef *value, bool (*eq)(const StringRef *, const StringRef *));

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
bool MiniMap_StringRef_StringRef_remove(MiniMap *map, const StringRef *key, bool (*eq)(const StringRef *, const StringRef *));

#undef MiniMap
