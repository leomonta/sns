//

#include "MiniMap_uint32_t_MessageProcessor.h"

#define MiniMap MiniMap_uint32_t_MessageProcessor

MiniMap MiniMap_uint32_t_MessageProcessor_make(const size_t initial_count) {

	MiniMap res;

	res.keys   = MiniVector_uint32_t_make(initial_count);
	res.values = MiniVector_MessageProcessor_make(initial_count);

	return res;
}

void MiniMap_uint32_t_MessageProcessor_destroy(MiniMap *map) {

	MiniVector_uint32_t_destroy(&map->keys);
	MiniVector_MessageProcessor_destroy(&map->values);
}

bool MiniMap_uint32_t_MessageProcessor_replace(MiniMap *map, const uint32_t *key, const MessageProcessor *value, bool (*eq)(const uint32_t *, const uint32_t *)) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (eq(map->keys.data + i, key)) {

			// replace it
			MiniVector_MessageProcessor_set(&map->values, i, value);
			return true;
		}
	}

	return false;
}

bool MiniMap_uint32_t_MessageProcessor_get(const MiniMap *map, const uint32_t *key, bool (*eq)(const uint32_t *, const uint32_t *), MessageProcessor* result) {

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

void MiniMap_uint32_t_MessageProcessor_set(MiniMap *map, const uint32_t *key, const MessageProcessor *value, bool (*eq)(const uint32_t *, const uint32_t *)) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (eq(map->keys.data + i, key)) {

			// replace it
			MiniVector_MessageProcessor_set(&map->values, i, value);
			return;
		}
	}

	// else append
	MiniVector_uint32_t_append(&map->keys, key);
	MiniVector_MessageProcessor_append(&map->values, value);
}

bool MiniMap_uint32_t_MessageProcessor_remove(MiniMap *map, const uint32_t *key, bool (*eq)(const uint32_t *, const uint32_t *)) {

	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (eq(map->keys.data + i, key)) {

			MiniVector_uint32_t_remove(&map->keys, i);
			MiniVector_MessageProcessor_remove(&map->values, i);

			return true;
		}
	}

	return false;
}

#undef MiniMap
