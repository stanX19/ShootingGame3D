#ifndef BASIC_UTILS_HPP
#define BASIC_UTILS_HPP
#include "includes.hpp"
#include "components.hpp"

template <typename T, typename... Ts>
std::vector<T> merge_vectors(const std::vector<T>& first, const std::vector<Ts>&... rest) {
	static_assert((std::is_same_v<T, Ts> && ...), "All arguments to merge_vectors must be vectors of the same type!");
	size_t total_size = first.size() + (rest.size() + ...);
	std::vector<T> combined;
	combined.reserve(total_size);
	combined.insert(combined.end(), first.begin(), first.end());
	(combined.insert(combined.end(), rest.begin(), rest.end()), ...);
	return combined;
}
#endif
