#include "StringRef.h"

#include <string.h>

int strncasecmp(const char *lhs, const char *rhs, size_t n) {

	int diff = 0;

	for (size_t i = 0 ; i < n; ++i) {
		char l = lhs[i];
		char r = rhs[i];
		if (l >= 'A' && l <= 'z') {
			l &= 0b01011111;
		}
		if (r >= 'A' && r <= 'z') {
			r &= 0b01011111;
		}
		diff = l - r;
		if (diff != 0 || l == '\0' || r == '\0'){
			return diff;
		}
	}

	return diff;

}

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

bool equal_case_StringOwn(const StringOwn *lhs, const StringOwn *rhs) {
	StringView lhs_v = {lhs->str, lhs->len};
	StringView rhs_v = {rhs->str, rhs->len};
	return equal_case_StringView(&lhs_v, &rhs_v);
}

bool equal_case_StringRef(const StringRef *lhs, const StringRef *rhs) {
	StringView lhs_v = {lhs->str, lhs->len};
	StringView rhs_v = {rhs->str, rhs->len};
	return equal_case_StringView(&lhs_v, &rhs_v);
}

bool equal_case_StringView(const StringView *lhs, const StringView *rhs) {
	if (lhs->len != rhs->len) {
		return false;
	}

	return strncasecmp(lhs->str, rhs->str, lhs->len) == 0;
}
