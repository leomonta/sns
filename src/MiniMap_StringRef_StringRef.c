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
	RET_IF_ERR(err) // just return, make failed
	err = MiniVector_StringRef_make(initial_count, &vec->values);
	if (err > NO_OP) {
		auto err2 = MiniVector_StringRef_destroy(&vec->keys);
		RET_IF_ERR(err2) // MiniVector_destroy can't fail (yet) so this is essentially a no op
		return err;
	}

	return NO_ERROR;
}

ERRFUNC MiniMap_StringRef_StringRef_destroy(MiniMap *map) {


	auto err = MiniVector_StringRef_destroy(&map->keys);
	auto err2 = MiniVector_StringRef_destroy(&map->values);

	RET_IF_ERR(err) // MiniVector_destroy can't fail, but even if it could
	RET_IF_ERR(err2) // can't do much else

	map->eq_fun = nullptr;

	return NO_ERROR;
}

ERRFUNC MiniMap_StringRef_StringRef_replace(MiniMap *map, const StringRef *key, const StringRef *value) {

	if (value == nullptr) {
		llog(LOG_WARNING, "[MINIMAP] Trying to set a null element\n");
		return INVALID_ARGUMENT;
	}

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			auto err = MiniVector_StringRef_set(&map->values, i, value);
			RET_IF_ERR(err) // this should never return and error (since we check inputs before) but if it does the operation fails
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

			auto err = MiniVector_StringRef_get(&map->values, i, result);
			RET_IF_ERR(err) // this can only give index out of range, and that's impossible here
			return NO_ERROR;
		}
	}

	return NO_OP;
}

ERRFUNC MiniMap_StringRef_StringRef_get_by_index(const MiniMap *map, const size_t index, StringRef *key, StringRef *value) {

	if (index > map->values.count) {
		llog(LOG_WARNING, "[MINIMAP] %ld >= %ld Unreachable position\n", index, map->values.count);
		return INVALID_ARGUMENT;
	}

	auto a = MiniVector_StringRef_get(&map->values, index, value);
	RET_IF_ERR(a) // this should never return error
	auto b = MiniVector_StringRef_get(&map->keys, index, key);
	RET_IF_ERR(b) // and this neither

	if (a == NO_OP && b == NO_OP) {
		return NO_OP;
	}

	return NO_ERROR;
}

ERRFUNC MiniMap_StringRef_StringRef_set(MiniMap *map, const StringRef *key, const StringRef *value) {

	if (value == nullptr) {
		llog(LOG_WARNING, "[MINIMAP] Trying to set a null element\n");
		return INVALID_ARGUMENT;
	}

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			auto err = MiniVector_StringRef_set(&map->values, i, value);
			RET_IF_ERR(err) // the index is always correct so no errors here
			return NO_ERROR;
		}
	}

	// else append
	auto err = MiniVector_StringRef_append(&map->keys, key);
	RET_IF_ERR(err) // can't do much here, it can only fail if growing fails.
	err = MiniVector_StringRef_append(&map->values, value);
	RET_IF_ERR(err)

	return NO_ERROR;
}

ERRFUNC MiniMap_StringRef_StringRef_remove(MiniMap *map, const StringRef *key) {

	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			auto err = MiniVector_StringRef_remove(&map->keys, i);
			RET_IF_ERR(err) // can't fail
			err = MiniVector_StringRef_remove(&map->values, i);
			RET_IF_ERR(err)

			return NO_ERROR;
		}
	}

	return NO_OP;
}

#undef MiniMap
