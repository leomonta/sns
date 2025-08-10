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
#include <stddef.h>
#include <string.h>

#define TO_STRINGREF(str) \
	{str, sizeof(str)}

#define CAST_STRINGREF(str) (StringRef){str, strlen(str)}

// Owning string
typedef struct {
	char  *str;
	size_t len;
} StringOwn;

// Non owning substring
typedef struct {
	const char *str;
	size_t      len;
} StringRef;

bool equal_StringOwn(const StringOwn *lhs, const StringOwn *rhs);
bool equal_StringRef(const StringRef *lhs, const StringRef *rhs);
