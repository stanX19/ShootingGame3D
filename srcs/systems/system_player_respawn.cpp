#include "systems.hpp"
#include "entities.hpp"

void ecs_systems::playerRespawn(GameContext &context, [[maybe_unused]] float dt) {
	if (!context.registry.valid(context.currentPlayer))
		spawnPlayer(context);
}