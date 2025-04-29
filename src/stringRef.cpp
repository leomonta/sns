#include "stringRef.hpp"

#include <cstring>

bool equal(const stringOwn *lhs, const stringOwn *rhs) {
	if (lhs->len != rhs->len) {
		return false;
	}

	return strncmp(lhs->str, rhs->str, lhs->len);
}

bool equal(const stringRef *lhs, const stringRef *rhs) {
	if (lhs->len != rhs->len) {
		return false;
	}

	return strncmp(lhs->str, rhs->str, lhs->len);
}
