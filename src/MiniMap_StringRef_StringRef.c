//

#include "MiniMap_StringRef_StringRef.h"

#define MiniMap MiniMap_StringRef_StringRef

ERRFUNC MiniMap_StringRef_StringRef_make(const size_t initial_count, bool (*eq)(const StringRef *, const StringRef *), MiniMap *vec) {

	*vec = (MiniMap){
		.keys   = {},
		.values = {},
		.eq_fun = eq,
	};
	auto err = MiniVector_StringRef_make(initial_count, &vec->keys);
	RET_IF_ERR(err)
	err = MiniVector_StringRef_make(initial_count, &vec->values);
	RET_IF_ERR(err)

	return NO_ERROR;
}

ERRFUNC MiniMap_StringRef_StringRef_destroy(MiniMap *map) {

	auto err = MiniVector_StringRef_destroy(&map->keys);
	RET_IF_ERR(err)
	err = MiniVector_StringRef_destroy(&map->values);
	RET_IF_ERR(err)

	return NO_ERROR;
}

ERRFUNC MiniMap_StringRef_StringRef_replace(MiniMap *map, const StringRef *key, const StringRef *value) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			auto err = MiniVector_StringRef_set(&map->values, i, value);
			RET_IF_ERR(err)
			return NO_ERROR;
		}
	}

	return NO_OP;
}

ERRFUNC MiniMap_StringRef_StringRef_get(const MiniMap *map, const StringRef *key, StringRef *result) {

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

ERRFUNC MiniMap_StringRef_StringRef_get_by_index(const MiniMap *map, const size_t index, StringRef *key, StringRef *value) {

	auto a = MiniVector_StringRef_get(&map->values, index, value);
	RET_IF_ERR(a)
	auto b = MiniVector_StringRef_get(&map->keys, index, key);
	RET_IF_ERR(b)

	if (a == NO_OP && b == NO_OP) {
		return NO_OP;
	}

	return NO_ERROR;
}

ERRFUNC MiniMap_StringRef_StringRef_set(MiniMap *map, const StringRef *key, const StringRef *value) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			auto err = MiniVector_StringRef_set(&map->values, i, value);
			RET_IF_ERR(err)
			return NO_ERROR;
		}
	}

	// else append
	auto err = MiniVector_StringRef_append(&map->keys, key);
	RET_IF_ERR(err)
	err = MiniVector_StringRef_append(&map->values, value);
	RET_IF_ERR(err)

	return NO_ERROR;
}

ERRFUNC MiniMap_StringRef_StringRef_remove(MiniMap *map, const StringRef *key) {

	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			auto err = MiniVector_StringRef_remove(&map->keys, i);
			RET_IF_ERR(err)
			err = MiniVector_StringRef_remove(&map->values, i);
			RET_IF_ERR(err)

			return NO_ERROR;
		}
	}

	return NO_OP;
}

#undef MiniMap
