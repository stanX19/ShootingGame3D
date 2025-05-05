#include "shoot_3d.hpp"

void ecs_system::enemy_aim(entt::registry &registry)
{
	auto playerView = registry.view<Player, Position>();
	if (playerView.begin() == playerView.end())
		return;

	Position playerPos = playerView.get<Position>(*playerView.begin());

	auto view = registry.view<Enemy, Position, BulletWeapon>();

	for (auto entity : view)
	{
		Position &position = view.get<Position>(entity);
		BulletWeapon &bulletWeapon = view.get<BulletWeapon>(entity);

		Vector3 toPlayer = Vector3Subtract(playerPos.value, position.value);
		float dist = Vector3Length(toPlayer);

		bulletWeapon.firing = (dist < 250.0f);

		if (!bulletWeapon.firing)
			continue;
	}
}
