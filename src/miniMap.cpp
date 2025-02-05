#include "miniMap.hpp"

#include "miniVector.hpp"

template <typename K, typename V>
bool miniMap::replace(const miniMap<K, V> *map, const K *key, const V *value) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->keys.data[i] == key) {

			// replace it
			miniVector::set(map->values, i, value);
			return true;
		}
	}

	return false;
}

template <typename K, typename V>
V miniMap::get( const miniMap<K, V> *map, const K *key) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->keys.data[i] == key) {

			return map->values.data[i];
		}
	}

	return nullptr;
}

template <typename K, typename V>
void miniMap::set(const miniMap<K, V> *map, const K *key, const V *value) {

	// find it
	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->keys.data[i] == key) {

			// replace it
			miniVector::set(map->values, i, value);
			return;
		}
	}

	// else append
	miniVector::append(map->keys, key);
	miniVector::append(map->values, value);
}

template <typename K, typename V>
bool miniMap::remove(const miniMap<K, V> *map, const K *key) {

	for (size_t i = 0; i < map->keys.count; ++i) {

		// if found
		if (map->keys.data[i] == key) {

			miniVector::remove(map->keys, i);
			miniVector::remove(map->values, i);

			return true;
		}
	}

	return false;
}
