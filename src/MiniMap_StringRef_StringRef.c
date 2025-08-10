//

#include "MiniMap_StringRef_StringRef.h"

#define MiniMap MiniMap_StringRef_StringRef

MiniMap MiniMap_StringRef_StringRef_make(const size_t initial_count) {

	MiniMap res;

	res.keys   = MiniVector_StringRef_make(initial_count);
	res.values = MiniVector_StringRef_make(initial_count);

	return res;
}

void MiniMap_StringRef_StringRef_destroy(MiniMap *map) {

	MiniVector_StringRef_destroy(&map->keys);
	MiniVector_StringRef_destroy(&map->values);
}

bool MiniMap_StringRef_StringRef_replace(MiniMap *map, const StringRef *key, const StringRef *value, bool (*eq)(const StringRef *, const StringRef *)) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (eq(map->keys.data + i, key)) {

			// replace it
			MiniVector_StringRef_set(&map->values, i, value);
			return true;
		}
	}

	return false;
}

bool MiniMap_StringRef_StringRef_get(const MiniMap *map, const StringRef *key, bool (*eq)(const StringRef *, const StringRef *), StringRef* result) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (eq(map->keys.data + i, key)) {

			*result = *(map->values.data + i);
			return true;
		}
	}

	return false;
}

void MiniMap_StringRef_StringRef_set(MiniMap *map, const StringRef *key, const StringRef *value, bool (*eq)(const StringRef *, const StringRef *)) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (eq(map->keys.data + i, key)) {

			// replace it
			MiniVector_StringRef_set(&map->values, i, value);
			return;
		}
	}

	// else append
	MiniVector_StringRef_append(&map->keys, key);
	MiniVector_StringRef_append(&map->values, value);
}

bool MiniMap_StringRef_StringRef_remove(MiniMap *map, const StringRef *key, bool (*eq)(const StringRef *, const StringRef *)) {

	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (eq(map->keys.data + i, key)) {

			MiniVector_StringRef_remove(&map->keys, i);
			MiniVector_StringRef_remove(&map->values, i);

			return true;
		}
	}

	return false;
}

#undef MiniMap
