//

#include "MiniMap_StringRef_MessageProcessor.h"

#define MiniMap MiniMap_StringRef_MessageProcessor

MiniMap MiniMap_StringRef_MessageProcessor_make(const size_t initial_count, bool (*eq)(const StringRef *, const StringRef *)) {

	MiniMap res;

	res.keys   = MiniVector_StringRef_make(initial_count);
	res.values = MiniVector_MessageProcessor_make(initial_count);
	res.eq_fun = eq;

	return res;
}

void MiniMap_StringRef_MessageProcessor_destroy(MiniMap *map) {

	MiniVector_StringRef_destroy(&map->keys);
	MiniVector_MessageProcessor_destroy(&map->values);
}

bool MiniMap_StringRef_MessageProcessor_replace(MiniMap *map, const StringRef *key, const MessageProcessor *value) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			MiniVector_MessageProcessor_set(&map->values, i, value);
			return true;
		}
	}

	return false;
}

bool MiniMap_StringRef_MessageProcessor_get(const MiniMap *map, const StringRef *key, MessageProcessor *result) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			*result = *(map->values.data + i);
			return true;
		}
	}

	return false;
}

void MiniMap_StringRef_MessageProcessor_set(MiniMap *map, const StringRef *key, const MessageProcessor *value) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			MiniVector_MessageProcessor_set(&map->values, i, value);
			return;
		}
	}

	// else append
	MiniVector_StringRef_append(&map->keys, key);
	MiniVector_MessageProcessor_append(&map->values, value);
}

bool MiniMap_StringRef_MessageProcessor_remove(MiniMap *map, const StringRef *key) {

	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			MiniVector_StringRef_remove(&map->keys, i);
			MiniVector_MessageProcessor_remove(&map->values, i);

			return true;
		}
	}

	return false;
}

#undef MiniMap
