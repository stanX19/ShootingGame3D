#include "color_utils.hpp"
#include "game_config.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <unordered_map>

Color colorRevert(Color a)
{
	Color result;
	result.r = (unsigned char)(255 - a.r);
	result.g = (unsigned char)(255 - a.g);
	result.b = (unsigned char)(255 - a.b);
	result.a = (unsigned char)(a.a);
	return result;
}

Color color_utils::parseColor(const nlohmann::json &j, Color defaultColor)
{
	if (j.is_array() && j.size() >= 3) {
		const int r = std::clamp(j[0].get<int>(), 0, 255);
		const int g = std::clamp(j[1].get<int>(), 0, 255);
		const int b = std::clamp(j[2].get<int>(), 0, 255);
		const int a = (j.size() > 3) ? std::clamp(j[3].get<int>(), 0, 255) : 255;
		return Color{
			static_cast<unsigned char>(r),
			static_cast<unsigned char>(g),
			static_cast<unsigned char>(b),
			static_cast<unsigned char>(a)
		};
	}

	if (!j.is_string()) {
		return defaultColor;
	}

	std::string lowerStr = j.get<std::string>();
	std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});

	static const std::unordered_map<std::string, Color> colorMap = {
		{"gray", GRAY},
		{"white", WHITE},
		{"yellow", YELLOW},
		{"orange", ORANGE},
		{"skyblue", SKYBLUE},
		{"red", RED},
		{"green", GREEN},
		{"blue", BLUE},
		{"black", BLACK},
		{"magenta", MAGENTA},
		{"gold", GOLD},
		{"violet", VIOLET}
	};

	const auto it = colorMap.find(lowerStr);
	if (it != colorMap.end()) {
		return it->second;
	}

	return defaultColor;
}

Color color_utils::getWeaponColor(
	const GameConfig &weaponCfg,
	const GameConfig &globalCfg,
	const std::string &category,
	const std::string &key,
	Color defaultColor
) {
	// 1. Weapon-specific override
	const auto &weaponJson = weaponCfg.getJson();
	if (weaponJson.contains(key)) {
		return parseColor(weaponJson[key], defaultColor);
	}

	// 2. Template-level default
	const auto templateSection = globalCfg.getSection("weapons.templates." + category);
	if (templateSection.contains(key)) {
		return parseColor(templateSection[key], defaultColor);
	}

	// 3. Category-level default (legacy)
	const auto categorySection = globalCfg.getSection("weapons." + category);
	if (categorySection.contains(key)) {
		return parseColor(categorySection[key], defaultColor);
	}

	// 4. Fallback
	return defaultColor;
}
