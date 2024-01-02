#pragma once

#include "miniVector.hpp"

namespace hmap {

	template <typename T, typename U>
	struct hmap {

		miniVector::miniVector<T> keys;
		miniVector::miniVector<U> values;
	};

	template <typename T, typename U>
	void insert(hmap<T, U>, T key, U element);

	template <typename T, typename U>
	U get(hmap<T, U>, T key);
} // namespace hmap
