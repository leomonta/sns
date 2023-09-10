#pragma once

#include <functional>
#include <stddef.h>

#define TO_STRINGREF(str) {str, strlen(str)}

// express a substring by referencing another c string
typedef struct stringRef {
	const char *str;
	size_t      len;

	bool operator==(const stringRef &sr) const {
		return (sr.str == str && sr.len == len);
	}
} stringRef;


namespace std {
	template <>
	struct hash<stringRef> {

		size_t operator()(const stringRef &k) const {
			// Compute individual hash values for two data members and combine them using XOR and bit shifting

			// #k.x 0 #k.y
			auto ptr = reinterpret_cast<size_t>(k.str) << 2;
			return (ptr * 10 + k.len);
		}
	};

	template <>
	struct equal_to<stringRef> {

		bool operator()(const stringRef &rhs, const stringRef &lhs) const {
			return rhs == lhs;
		}
	};
} // namespace std