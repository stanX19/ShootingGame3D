#pragma once

#include "includes.hpp"
#include "json.hpp"
#include <string>

class GameConfig;

Color colorRevert(Color a);

namespace color_utils {
	Color parseColor(const nlohmann::json &j, Color defaultColor);
	Color getWeaponColor(
		const GameConfig &weaponCfg,
		const GameConfig &globalCfg,
		const std::string &category,
		const std::string &key,
		Color defaultColor
	);
}
