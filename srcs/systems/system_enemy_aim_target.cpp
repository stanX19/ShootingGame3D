#include "shoot_3d.hpp"

void ecs_systems::enemyAimTarget(entt::registry &registry)
{
	auto playerView = registry.view<tag::Player, Position>();
	if (playerView.begin() == playerView.end())
		return;

	entt::entity player = *playerView.begin();
	Position playerPos = playerView.get<Position>(player);

	auto view = registry.view<tag::Enemy, Position, BulletWeapon, AimTarget>();

	for (auto entity : view)
	{
		Position &position = view.get<Position>(entity);
		BulletWeapon &bulletWeapon = view.get<BulletWeapon>(entity);
		AimTarget &aimTarget = view.get<AimTarget>(entity);

		Vector3 toPlayer = Vector3Subtract(playerPos.value, position.value);
		float dist = Vector3Length(toPlayer);

		bulletWeapon.firing = (dist < 100.0f);

		if (!bulletWeapon.firing)
			continue;

		if (!aimTargetExists(registry, aimTarget))
			aimTarget.entity = player;
	}
}
