//

#include "MiniMap_u_char_StringOwn.h"

#define MiniMap MiniMap_u_char_StringOwn

ERRFUNC MiniMap_u_char_StringOwn_make(const size_t initial_count, bool (*eq)(const u_char *, const u_char *), MiniMap *vec) {

	*vec = (MiniMap){
		.keys   = {},
		.values = {},
		.eq_fun = eq,
	};
	auto err = MiniVector_u_char_make(initial_count, &vec->keys);
	RET_IF_ERR(err)
	err = MiniVector_StringOwn_make(initial_count, &vec->values);
	RET_IF_ERR(err)

	return NO_ERROR;
}

ERRFUNC MiniMap_u_char_StringOwn_destroy(MiniMap *map) {

	auto err = MiniVector_u_char_destroy(&map->keys);
	RET_IF_ERR(err)
	err = MiniVector_StringOwn_destroy(&map->values);
	RET_IF_ERR(err)

	return NO_ERROR;
}

ERRFUNC MiniMap_u_char_StringOwn_replace(MiniMap *map, const u_char *key, const StringOwn *value) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			auto err = MiniVector_StringOwn_set(&map->values, i, value);
			RET_IF_ERR(err)
			return NO_ERROR;
		}
	}

	return NO_OP;
}

ERRFUNC MiniMap_u_char_StringOwn_get(const MiniMap *map, const u_char *key, StringOwn *result) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			*result = *(map->values.data + i);
			return NO_ERROR;
		}
	}

	return NO_OP;
}

ERRFUNC MiniMap_u_char_StringOwn_get_by_index(const MiniMap *map, const size_t index, u_char *key, StringOwn *value) {

	auto a = MiniVector_StringOwn_get(&map->values, index, value);
	RET_IF_ERR(a)
	auto b = MiniVector_u_char_get(&map->keys, index, key);
	RET_IF_ERR(b)

	if (a == NO_OP && b == NO_OP) {
		return NO_OP;
	}

	return NO_ERROR;
}

ERRFUNC MiniMap_u_char_StringOwn_set(MiniMap *map, const u_char *key, const StringOwn *value) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			auto err = MiniVector_StringOwn_set(&map->values, i, value);
			RET_IF_ERR(err)
			return NO_ERROR;
		}
	}

	// else append
	auto err = MiniVector_u_char_append(&map->keys, key);
	RET_IF_ERR(err)
	err = MiniVector_StringOwn_append(&map->values, value);
	RET_IF_ERR(err)

	return NO_ERROR;
}

ERRFUNC MiniMap_u_char_StringOwn_remove(MiniMap *map, const u_char *key) {

	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			auto err = MiniVector_u_char_remove(&map->keys, i);
			RET_IF_ERR(err)
			err = MiniVector_StringOwn_remove(&map->values, i);
			RET_IF_ERR(err)

			return NO_ERROR;
		}
	}

	return NO_OP;
}

#undef MiniMap
