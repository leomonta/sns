//template <K, V>

#include "MiniMap_#K#_#V#.h"

#define MiniMap MiniMap_#K#_#V#

MiniMap MiniMap_#K#_#V#_make(const size_t initial_count, bool (*eq)(const K *, const K *)) {

	MiniMap res;

	res.keys   = MiniVector_#K#_make(initial_count);
	res.values = MiniVector_#V#_make(initial_count);
	res.eq_fun = eq;

	return res;
}

void MiniMap_#K#_#V#_destroy(MiniMap *map) {

	MiniVector_#K#_destroy(&map->keys);
	MiniVector_#V#_destroy(&map->values);
}

bool MiniMap_#K#_#V#_replace(MiniMap *map, const K *key, const V *value) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			MiniVector_#V#_set(&map->values, i, value);
			return true;
		}
	}

	return false;
}

bool MiniMap_#K#_#V#_get(const MiniMap *map, const K *key, V *result) {

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

void MiniMap_#K#_#V#_set(MiniMap *map, const K *key, const V *value) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			MiniVector_#V#_set(&map->values, i, value);
			return;
		}
	}

	// else append
	MiniVector_#K#_append(&map->keys, key);
	MiniVector_#V#_append(&map->values, value);
}

bool MiniMap_#K#_#V#_remove(MiniMap *map, const K *key) {

	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			MiniVector_#K#_remove(&map->keys, i);
			MiniVector_#V#_remove(&map->values, i);

			return true;
		}
	}

	return false;
}

#undef MiniMap
