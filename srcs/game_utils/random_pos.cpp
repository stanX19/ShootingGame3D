#include "game_utils.hpp"

Vector3 game_utils::randomPosOffCombat(Vector3 playerPos, float combatDist) {
	Vector3 pos;
	do
		pos = randomUnitVector3() * (combatDist + 1500);
	while (Vector3Distance(pos, playerPos) < combatDist + 1000);
	return pos;
}

Vector3 game_utils::randomPosInBox(Vector3 start, Vector3 end) {
	return Vector3{
		randomFloat(start.x, end.x),
		randomFloat(start.y, end.y),
		randomFloat(start.z, end.z)
	};
}

Vector3 game_utils::randomPosInBoxOffCombat(Vector3 start, Vector3 end, Vector3 playerPos, float combatDist) {
	const float minDist = combatDist * 1.5f;

	Vector3 pos = start;

	for (int i = 0; i < 100; i++) {
		pos = game_utils::randomPosInBox(start, end);
		if (Vector3Distance(pos, playerPos) >= minDist) {
			return pos;
		}
	}
	pos = randomUnitVector3() * minDist;
	return Vector3Clamp(pos, start, end);
}


Vector3 game_utils::randomPosInField(float arenaSize)
{
	float x = GetRandomValue(-arenaSize / 2 + 5, arenaSize / 2 - 5);
	float z = GetRandomValue(-arenaSize / 2 + 5, arenaSize / 2 - 5);
	float y = GetRandomValue(-arenaSize / 2 + 5, arenaSize / 2 - 5);
	return Vector3{x, y, z};
}