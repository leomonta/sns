#pragma once
/*
 * This is an abstraction to differentiate between owning strings and non owning strings
 *
 * Essentially:
 * A `stringOwn` is made by a `char*` and a `size_t` and represents an allocated area of memory, it may or may not be null terminated
 * there should be only one valid `stringOwn` pointing to a specific address at a time, and as such this should be allocated and freed
 *
 * A `stringRef` is made by a `const char*` and a `size_t` and represents a substring, it may or may not be allocated, null terminated, or at the start of the string
 * there can be as many valid `stringRef` pointing to a specific addreses at a time as needed, and as such this should not be allocated nor freed
 *
 * Usually a `stringOwn` is created and a `stringRef` references it.
 *
 * To copy a `stringOwn` new memory should be reserved and copied.
 */
#include <cstddef>

#define TO_STRINGREF(str) \
	{str, strlen(str)}

// Owning string
typedef struct stringOwn {
	char  *str = nullptr;
	size_t len = 0;
} stringOwn;

// Non owning substring
typedef struct stringRef {
	const char *str = nullptr;
	size_t      len = 0;
} stringRef;

bool equal(const stringOwn *lhs, const stringOwn *rhs);
bool equal(const stringRef *lhs, const stringRef *rhs);
