#pragma once

#include <cstddef>

#define TO_STRINGREF(str) \
	{str, strlen(str)}

// express a substring by referencing another c string
typedef struct stringRef {
	char  *str = nullptr;
	size_t len = 0;
} stringRef;

typedef struct stringRefConst {
	const char *str = nullptr;
	size_t      len = 0;
} stringRefConst;

bool equal(const stringRef *lhs, const stringRef *rhs);
bool equal(const stringRefConst *lhs, const stringRefConst *rhs);
