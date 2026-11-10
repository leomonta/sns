#pragma once

#include "errcode.h"

// 

#include "MiniVector_StringRef.h"
#include "MiniVector_MessageProcessor.h"



#define MiniMap MiniMap_StringRef_MessageProcessor

typedef struct {
	MiniVector_StringRef keys;
	MiniVector_MessageProcessor values;
	bool (*eq_fun)(const StringRef *, const StringRef *);
} MiniMap;

/**
 * Makes a MiniMap with a preallocated array of initial_count length
 *
 * @param[in] `initial_count` how many elements to preallocate, defaults to 10 if zero is specified
 * @param[in] `eq` a function to compare the equality of two keys
 * @param[out] `map` where to place the constructed MiniMap
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC MiniMap_StringRef_MessageProcessor_make(const size_t initial_count, bool (*eq)(const StringRef *, const StringRef *), MiniMap *map);

/**
 * Frees up all the resources allocated by the miniMap
 *
 * @param[in] `map` the MiniMap to destroy
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC MiniMap_StringRef_MessageProcessor_destroy(MiniMap *map);

/**
 * Replace the value at the given key with the given value.
 * If the key is not found no operation is performed and returns NO_OP
 *
 * @param[in] `map` the MiniMap to insert the values into
 * @param[in] `key` the key relative for the value, the key is applied directly, no hash function is applied, thus the hashing step should be done before
 * @param[in] `value` the value to associate to the given key
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC MiniMap_StringRef_MessageProcessor_replace(MiniMap *map, const StringRef *key, const MessageProcessor *value);

/**
 * Get the value corresponding to the giving key
 *
 * @param[in] `map` the map to get the value from
 * @param[in] `key` the key of the value to return, no hash function is applied, thus the hashing step should be done before
 * @param[out] `result` where to place the value associated with the given array
 * if NO_OP or another error returned this values should be ignored as there was no data to communicate back
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC MiniMap_StringRef_MessageProcessor_get(const MiniMap *map, const StringRef *key, MessageProcessor *result);

/**
 * Get the value corresponding to the giving key
 *
 * @param[in] `map` the map to get the value from
 * @param[in] `index` the index of the entry to return
 * @param[out] `key` the key at the specified index, no hash function is applied, thus the hashing step should be done before
 * if NO_OP or another error returned this values should be ignored as there was no data to communicate back
 * @param[out] `result` where to place the value associated with the given array
 * if NO_OP or another error returned this values should be ignored as there was no data to communicate back
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC MiniMap_StringRef_MessageProcessor_get_by_index(const MiniMap *map, const size_t index, StringRef *key, MessageProcessor *result);

/**
 * Insert a value into the map through its relative key
 * if the key is present replaces the stored value with the given one
 * else appends both key and value
 *
 * @param[in] `map` the MiniMap to set the values into
 * @param[in] `key` the key relative for the value, the key is applied directly, no hash function is applied, thus the hashing step should be done before
 * @param[in] `value` the value to associate to the given key
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC MiniMap_StringRef_MessageProcessor_set(MiniMap *map, const StringRef *key, const MessageProcessor *value);

/**
 * Attempts to remove the key and its relative value
 * return NO_OP if there is nothing to remove
 *
 * @param[in] `map` the MiniMap to remove the values from
 * @param[in] `key` the key relative for the value, yhe key is applied directly, no hash function is applied, thus the hashing step should be done before
 *
 * @return an error code describing if and which error occurred
 */
ERRFUNC MiniMap_StringRef_MessageProcessor_remove(MiniMap *map, const StringRef *key);

#undef MiniMap
