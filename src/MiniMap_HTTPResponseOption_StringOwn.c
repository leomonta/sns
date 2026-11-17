//

#include "MiniMap_HTTPResponseOption_StringOwn.h"

#define MiniMap MiniMap_HTTPResponseOption_StringOwn

ERRFUNC MiniMap_HTTPResponseOption_StringOwn_make(const size_t initial_count, bool (*eq)(const HTTPResponseOption *, const HTTPResponseOption *), MiniMap *vec) {

	*vec = (MiniMap){
		.keys   = {},
		.values = {},
		.eq_fun = eq,
	};
	auto err = MiniVector_HTTPResponseOption_make(initial_count, &vec->keys);
	_SNS_RET_IF_ERR(err) // just return, make failed
	err = MiniVector_StringOwn_make(initial_count, &vec->values);
	if (err > NO_OP) {
		auto err2 = MiniVector_HTTPResponseOption_destroy(&vec->keys);
		_SNS_RET_IF_ERR(err2) // MiniVector_destroy can't fail (yet) so this is essentially a no op
		return err;
	}

	return NO_ERROR;
}

ERRFUNC MiniMap_HTTPResponseOption_StringOwn_destroy(MiniMap *map) {


	auto err = MiniVector_HTTPResponseOption_destroy(&map->keys);
	auto err2 = MiniVector_StringOwn_destroy(&map->values);

	_SNS_RET_IF_ERR(err) // MiniVector_destroy can't fail, but even if it could
	_SNS_RET_IF_ERR(err2) // can't do much else

	map->eq_fun = nullptr;

	return NO_ERROR;
}

ERRFUNC MiniMap_HTTPResponseOption_StringOwn_replace(MiniMap *map, const HTTPResponseOption *key, const StringOwn *value) {

	if (value == nullptr) {
		llog(LOG_WARNING, "[MINIMAP] Trying to set a null element\n");
		return INVALID_ARGUMENT;
	}

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			auto err = MiniVector_StringOwn_set(&map->values, i, value);
			_SNS_RET_IF_ERR(err) // this should never return and error (since we check inputs before) but if it does the operation fails
			return NO_ERROR;
		}
	}

	return NO_OP;
}

ERRFUNC MiniMap_HTTPResponseOption_StringOwn_get(const MiniMap *map, const HTTPResponseOption *key, StringOwn *result) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			auto err = MiniVector_StringOwn_get(&map->values, i, result);
			_SNS_RET_IF_ERR(err) // this can only give index out of range, and that's impossible here
			return NO_ERROR;
		}
	}

	return NO_OP;
}

ERRFUNC MiniMap_HTTPResponseOption_StringOwn_get_by_index(const MiniMap *map, const size_t index, HTTPResponseOption *key, StringOwn *value) {

	if (index > map->values.count) {
		llog(LOG_WARNING, "[MINIMAP] %ld >= %ld Unreachable position\n", index, map->values.count);
		return INVALID_ARGUMENT;
	}

	auto a = MiniVector_StringOwn_get(&map->values, index, value);
	_SNS_RET_IF_ERR(a) // this should never return error
	auto b = MiniVector_HTTPResponseOption_get(&map->keys, index, key);
	_SNS_RET_IF_ERR(b) // and this neither

	if (a == NO_OP && b == NO_OP) {
		return NO_OP;
	}

	return NO_ERROR;
}

ERRFUNC MiniMap_HTTPResponseOption_StringOwn_set(MiniMap *map, const HTTPResponseOption *key, const StringOwn *value) {

	if (value == nullptr) {
		llog(LOG_WARNING, "[MINIMAP] Trying to set a null element\n");
		return INVALID_ARGUMENT;
	}

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			// replace it
			auto err = MiniVector_StringOwn_set(&map->values, i, value);
			_SNS_RET_IF_ERR(err) // the index is always correct so no errors here
			return NO_ERROR;
		}
	}

	// else append
	auto err = MiniVector_HTTPResponseOption_append(&map->keys, key);
	_SNS_RET_IF_ERR(err) // can't do much here, it can only fail if growing fails.
	err = MiniVector_StringOwn_append(&map->values, value);
	_SNS_RET_IF_ERR(err)

	return NO_ERROR;
}

ERRFUNC MiniMap_HTTPResponseOption_StringOwn_remove(MiniMap *map, const HTTPResponseOption *key) {

	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->eq_fun(map->keys.data + i, key)) {

			auto err = MiniVector_HTTPResponseOption_remove(&map->keys, i);
			_SNS_RET_IF_ERR(err) // can't fail
			err = MiniVector_StringOwn_remove(&map->values, i);
			_SNS_RET_IF_ERR(err)

			return NO_ERROR;
		}
	}

	return NO_OP;
}

#undef MiniMap
