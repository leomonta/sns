#include "hmap.hpp"
#include "miniVector.hpp"

template <typename T, typename U>
void hmap::insert(hmap<T, U> map, T key, U element) {

	// find it 
	for (size_t i = 0; i < map.keys.count; ++i) {

		// if found
		if (map.keys.data[i] == key) {

			// replace it
			miniVector::set(map.values, i, element);
			return;
		}
	}

	// else append
	miniVector::append(map.keys, key);
	miniVector::append(map.values, element);
}

template <typename T, typename U>
U hmap::get(hmap<T, U> map, T key) {


	// find it 
	for (size_t i = 0; i < map.keys.count; ++i) {

		// if found
		if (map.keys.data[i] == key) {

			return map.values.data[i];
		}
	}

	return nullptr;
	
}
