#include "StringRef.h"

#include <string.h>

bool equal_StringOwn(const StringOwn *lhs, const StringOwn *rhs) {
	StringView lhs_v = {lhs->str, lhs->len};
	StringView rhs_v = {rhs->str, rhs->len};
	return equal_StringView(&lhs_v, &rhs_v);
}

bool equal_StringRef(const StringRef *lhs, const StringRef *rhs) {
	StringView lhs_v = {lhs->str, lhs->len};
	StringView rhs_v = {rhs->str, rhs->len};
	return equal_StringView(&lhs_v, &rhs_v);
}

bool equal_StringView(const StringView *lhs, const StringView *rhs) {
	if (lhs->len != rhs->len) {
		return false;
	}

	return strncmp(lhs->str, rhs->str, lhs->len) == 0;
}
