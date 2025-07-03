#include "systems.hpp"
#include "entities.hpp"

void ecs_systems::playerRespawn(GameContext &context) {
	if (!context.registry.valid(context.currentPlayer))
		spawnPlayer(context);
}