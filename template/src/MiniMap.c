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
	SNS_RET_IF_ERR(err) // just return, make failed
	err = MiniVector_#V#_make(initial_count, &vec->values);
	if (err > NO_OP) {
		auto err2 = MiniVector_#K#_destroy(&vec->keys);
		SNS_RET_IF_ERR(err2) // MiniVector_destroy can't fail (yet) so this is essentially a no op
		return err;
	}

	return NO_ERROR;
}

ERRFUNC MiniMap_#K#_#V#_destroy(MiniMap *map) {


	auto err = MiniVector_#K#_destroy(&map->keys);
	auto err2 = MiniVector_#V#_destroy(&map->values);

	SNS_RET_IF_ERR(err) // MiniVector_destroy can't fail, but even if it could
	SNS_RET_IF_ERR(err2) // can't do much else

	map->eq_fun = nullptr;

	return NO_ERROR;
}

ERRFUNC MiniMap_#K#_#V#_replace(MiniMap *map, const K *key, const V *value) {

	if (value == nullptr) {
		llog(LOG_WARNING, "[MINIMAP] Trying to set a null element\n");
		return INVALID_ARGUMENT;
	}

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			auto err = MiniVector_#V#_set(&map->values, i, value);
			SNS_RET_IF_ERR(err) // this should never return and error (since we check inputs before) but if it does the operation fails
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

			auto err = MiniVector_#V#_get(&map->values, i, result);
			SNS_RET_IF_ERR(err) // this can only give index out of range, and that's impossible here
			return NO_ERROR;
		}
	}

	return NO_OP;
}

ERRFUNC MiniMap_#K#_#V#_get_by_index(const MiniMap *map, const size_t index, K *key, V *value) {

	if (index > map->values.count) {
		llog(LOG_WARNING, "[MINIMAP] %ld >= %ld Unreachable position\n", index, map->values.count);
		return INVALID_ARGUMENT;
	}

	auto a = MiniVector_#V#_get(&map->values, index, value);
	SNS_RET_IF_ERR(a) // this should never return error
	auto b = MiniVector_#K#_get(&map->keys, index, key);
	SNS_RET_IF_ERR(b) // and this neither

	if (a == NO_OP && b == NO_OP) {
		return NO_OP;
	}

	return NO_ERROR;
}

ERRFUNC MiniMap_#K#_#V#_set(MiniMap *map, const K *key, const V *value) {

	if (value == nullptr) {
		llog(LOG_WARNING, "[MINIMAP] Trying to set a null element\n");
		return INVALID_ARGUMENT;
	}

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			auto err = MiniVector_#V#_set(&map->values, i, value);
			SNS_RET_IF_ERR(err) // the index is always correct so no errors here
			return NO_ERROR;
		}
	}

	// else append
	auto err = MiniVector_#K#_append(&map->keys, key);
	SNS_RET_IF_ERR(err) // can't do much here, it can only fail if growing fails.
	err = MiniVector_#V#_append(&map->values, value);
	SNS_RET_IF_ERR(err)

	return NO_ERROR;
}

ERRFUNC MiniMap_#K#_#V#_remove(MiniMap *map, const K *key) {

	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			auto err = MiniVector_#K#_remove(&map->keys, i);
			SNS_RET_IF_ERR(err) // can't fail
			err = MiniVector_#V#_remove(&map->values, i);
			SNS_RET_IF_ERR(err)

			return NO_ERROR;
		}
	}

	return NO_OP;
}

#undef MiniMap
