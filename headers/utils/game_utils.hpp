#ifndef GAME_UTILS_HPP
#define GAME_UTILS_HPP
#include "includes.hpp"
#include "utils.hpp"

namespace game_utils {
	Vector3 randomPosInField(float arenaSize);
	Vector3 randomPosOffCombat(Vector3 playerPos, float combatDist);
	Vector3 randomPosInBox(Vector3 start, Vector3 end);
	Vector3 randomPosInBoxOffCombat(Vector3 start, Vector3 end, Vector3 playerPos, float combatDist);
};

#endif