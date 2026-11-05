#include "StringRef.h"

#include <string.h>

bool equal_StringOwn(const StringOwn *lhs, const StringOwn *rhs) {
	if (lhs->len != rhs->len) {
		return false;
	}

	return strncmp(lhs->str, rhs->str, lhs->len) == 0;
}

bool equal_StringRef(const StringRef *lhs, const StringRef *rhs) {
	if (lhs->len != rhs->len) {
		return false;
	}

	return strncmp(lhs->str, rhs->str, lhs->len) == 0;
}

bool equal_StringView(const StringView *lhs, const StringView *rhs) {
	if (lhs->len != rhs->len) {
		return false;
	}

	return strncmp(lhs->str, rhs->str, lhs->len) == 0;
}
