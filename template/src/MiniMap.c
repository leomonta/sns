//template <K, V>

#include "MiniMap_#K#_#V#.h"

#define MiniMap MiniMap_#K#_#V#

ERRFUNC MiniMap_#K#_#V#_make(const size_t initial_count, bool (*eq)(const K *, const K *), MiniMap *vec) {

	*vec = (MiniMap){
		.keys   = {},
		.values = {},
		.eq_fun = eq,
	};
	auto err = MiniVector_#K#_make(initial_count, &vec->keys);
	RET_IF_ERR(err)
	err = MiniVector_#V#_make(initial_count, &vec->values);
	RET_IF_ERR(err)

	return NO_ERROR;
}

ERRFUNC MiniMap_#K#_#V#_destroy(MiniMap *map) {

	auto err = MiniVector_#K#_destroy(&map->keys);
	RET_IF_ERR(err)
	err = MiniVector_#V#_destroy(&map->values);
	RET_IF_ERR(err)

	return NO_ERROR;
}

ERRFUNC MiniMap_#K#_#V#_replace(MiniMap *map, const K *key, const V *value) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			auto err = MiniVector_#V#_set(&map->values, i, value);
			RET_IF_ERR(err)
			return NO_ERROR;
		}
	}

	return NO_OP;
}

ERRFUNC MiniMap_#K#_#V#_get(const MiniMap *map, const K *key, V *result) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			auto err = MiniVEctor_#V#_get(&map->values, i, result);
			RET_IF_ERR(err)
			return NO_ERROR;
		}
	}

	return NO_OP;
}

ERRFUNC MiniMap_#K#_#V#_get_by_index(const MiniMap *map, const size_t index, K *key, V *value) {

	auto a = MiniVector_#V#_get(&map->values, index, value);
	RET_IF_ERR(a)
	auto b = MiniVector_#K#_get(&map->keys, index, key);
	RET_IF_ERR(b)

	if (a == NO_OP && b == NO_OP) {
		return NO_OP;
	}

	return NO_ERROR;
}

ERRFUNC MiniMap_#K#_#V#_set(MiniMap *map, const K *key, const V *value) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			auto err = MiniVector_#V#_set(&map->values, i, value);
			RET_IF_ERR(err)
			return NO_ERROR;
		}
	}

	// else append
	auto err = MiniVector_#K#_append(&map->keys, key);
	RET_IF_ERR(err)
	err = MiniVector_#V#_append(&map->values, value);
	RET_IF_ERR(err)

	return NO_ERROR;
}

ERRFUNC MiniMap_#K#_#V#_remove(MiniMap *map, const K *key) {

	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			auto err = MiniVector_#K#_remove(&map->keys, i);
			RET_IF_ERR(err)
			err = MiniVector_#V#_remove(&map->values, i);
			RET_IF_ERR(err)

			return NO_ERROR;
		}
	}

	return NO_OP;
}

#undef MiniMap
