#include "stringRef.hpp"

#include <cstring>

bool equal(const stringRef *lhs, const stringRef *rhs) {
	if (lhs->len != rhs->len) {
		return false;
	}

	return strncmp(lhs->str, rhs->str, lhs->len);
}

bool equal(const stringRefConst *lhs, const stringRefConst *rhs) {
	if (lhs->len != rhs->len) {
		return false;
	}

	return strncmp(lhs->str, rhs->str, lhs->len);
}
