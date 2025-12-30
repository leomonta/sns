//

#include "MiniMap_u_char_StringOwn.h"

#define MiniMap MiniMap_u_char_StringOwn

MiniMap MiniMap_u_char_StringOwn_make(const size_t initial_count, bool (*eq)(const u_char *, const u_char *)) {

	MiniMap res;

	res.keys   = MiniVector_u_char_make(initial_count);
	res.values = MiniVector_StringOwn_make(initial_count);
	res.eq_fun = eq;

	return res;
}

void MiniMap_u_char_StringOwn_destroy(MiniMap *map) {

	MiniVector_u_char_destroy(&map->keys);
	MiniVector_StringOwn_destroy(&map->values);
}

bool MiniMap_u_char_StringOwn_replace(MiniMap *map, const u_char *key, const StringOwn *value) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			MiniVector_StringOwn_set(&map->values, i, value);
			return true;
		}
	}

	return false;
}

bool MiniMap_u_char_StringOwn_get(const MiniMap *map, const u_char *key, StringOwn *result) {

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

void MiniMap_u_char_StringOwn_set(MiniMap *map, const u_char *key, const StringOwn *value) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			MiniVector_StringOwn_set(&map->values, i, value);
			return;
		}
	}

	// else append
	MiniVector_u_char_append(&map->keys, key);
	MiniVector_StringOwn_append(&map->values, value);
}

bool MiniMap_u_char_StringOwn_remove(MiniMap *map, const u_char *key) {

	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			MiniVector_u_char_remove(&map->keys, i);
			MiniVector_StringOwn_remove(&map->values, i);

			return true;
		}
	}

	return false;
}

#undef MiniMap
