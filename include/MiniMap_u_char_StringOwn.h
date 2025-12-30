#pragma once

// 

#include "MiniVector_u_char.h"
#include "MiniVector_StringOwn.h"

#include "StringRef.h"

#define MiniMap MiniMap_u_char_StringOwn

typedef struct {
	MiniVector_u_char keys;
	MiniVector_StringOwn values;
	bool (*eq_fun)(const u_char *, const u_char *);
} MiniMap;

/**
 * Makes a MiniMap with a preallocated array of initial_count length
 *
 * @param[in] `initial_count` how many elements to preallocate, defaults to 10 if zero is specified
 * @param[in] `eq` a function to compare the equality of two keys
 *
 * @return a built MiniMap
 */
MiniMap MiniMap_u_char_StringOwn_make(const size_t initial_count, bool (*eq)(const u_char *, const u_char *));

/**
 * Frees up all the resources allocated by the miniMap
 *
 * @param[in] `map` the MiniMap to destroy
 */
void MiniMap_u_char_StringOwn_destroy(MiniMap *map);

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
bool MiniMap_u_char_StringOwn_replace(MiniMap *map, const u_char *key, const StringOwn *value);

/**
 * Get the value corresponding to the giving key
 *
 * @param[in] `map` the map to get the value from
 * @param[in] `key` the key of the value to return
 * @param[out] `result` where to place the value associated with the given array
 *
 * @return a pointer the value associated with the given key, or nullptr if the key is not present in the map
 */
bool MiniMap_u_char_StringOwn_get(const MiniMap *map, const u_char *key, StringOwn* result);

/**
 * Insert a value into the hash map trhough its relative key
 * if the key is present replaces the stored value with the given one
 * else appends both key and value
 *
 * @param[in] `map` the MiniMap to set the values into
 * @param[in] `key` the key relative for the value, the key is applied directly, no hash function is applied, thus the hashing step should be done before
 * @param[in] `value` the value to associate to the given key
 */
void MiniMap_u_char_StringOwn_set(MiniMap *map, const u_char *key, const StringOwn *value);

/**
 * Attempts to remove the key and its relative value
 * return true if succeedes
 *
 * @param[in] `map` the MiniMap to remove the values from
 * @param[in] `key` the key relative for the value, yhe key is applied directly, no hash function is applied, thus the hashing step should be done before
 *
 * @return if the key value pair has been removed
 */
bool MiniMap_u_char_StringOwn_remove(MiniMap *map, const u_char *key);

#undef MiniMap
