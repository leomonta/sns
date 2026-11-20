#include "StringRef.h"

#include <string.h>
#include <strings.h>

bool equal_StringOwn(const StringOwn *lhs, const StringOwn *rhs) {
	return equal_sized_strings(lhs->str, lhs->len, rhs->str, rhs->len);
}

bool equal_StringRef(const StringRef *lhs, const StringRef *rhs) {
	return equal_sized_strings(lhs->str, lhs->len, rhs->str, rhs->len);
}

bool equal_StringView(const StringView *lhs, const StringView *rhs) {
	return equal_sized_strings(lhs->str, lhs->len, rhs->str, rhs->len);
}

bool equal_sized_strings(const char *lhs, const size_t lhs_s, const char *rhs, const size_t rhs_s) {
	if (lhs_s != rhs_s) {
		return false;
	}

	return strncmp(lhs, rhs, lhs_s) == 0;
}

bool equal_case_StringOwn(const StringOwn *lhs, const StringOwn *rhs) {
	return equal_case_sized_strings(lhs->str, lhs->len, rhs->str, rhs->len);
}

bool equal_case_StringRef(const StringRef *lhs, const StringRef *rhs) {
	return equal_case_sized_strings(lhs->str, lhs->len, rhs->str, rhs->len);
}

bool equal_case_StringView(const StringView *lhs, const StringView *rhs) {
	return equal_case_sized_strings(lhs->str, lhs->len, rhs->str, rhs->len);
}

bool equal_case_sized_strings(const char *lhs, const size_t lhs_s, const char *rhs, const size_t rhs_s) {
	if (lhs_s != rhs_s) {
		return false;
	}

	return strncasecmp(lhs, rhs, lhs_s) == 0;
}
