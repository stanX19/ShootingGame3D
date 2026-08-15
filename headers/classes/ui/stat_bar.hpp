#ifndef UI_STAT_BAR_HPP
#define UI_STAT_BAR_HPP

#include "includes.hpp"

#include <algorithm>
#include <string>

namespace ui {

struct StatBar {
	std::string name;
	float value = 0.0f;
	float maximum = 1.0f;
	float getClampedValue() const;
	float getNormalizedValue() const;
	void draw(Rectangle bounds) const;
};

} // namespace ui

#endif // UI_STAT_BAR_HPP
