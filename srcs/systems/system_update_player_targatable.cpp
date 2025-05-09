#include "shoot_3d.hpp"

void ecs_systems::updatePlayerTargetable(entt::registry &registry) {
	auto playerView = registry.view<Player, Position>();

	if (playerView.begin() == playerView.end())
		return ;

	Vector3 playerPos = playerView.get<Position>(*playerView.begin()).value;
	auto view = registry.view<PlayerTargetable, Position>();

	for (auto entity : view)
	{
		PlayerTargetable &playerTargetable = view.get<PlayerTargetable>(entity);
		Position &position = view.get<Position>(entity);

		playerTargetable.toSelf = position.value - playerPos;
		playerTargetable.distance = Vector3Length(playerTargetable.toSelf) * 10;
	}
}