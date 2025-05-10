#include "shoot_3d.hpp"

void ecs_systems::enemyAimTarget(entt::registry &registry)
{
	auto playerView = registry.view<tag::Player, Position>();
	if (playerView.begin() == playerView.end())
		return;

	entt::entity player = *playerView.begin();

	auto view = registry.view<tag::Enemy, Position, Rotation, BulletWeapon, AimTarget>();

	for (auto entity : view)
	{
		Position &position = view.get<Position>(entity);
		Rotation &rotation = view.get<Rotation>(entity);
		BulletWeapon &bulletWeapon = view.get<BulletWeapon>(entity);
		AimTarget &aimTarget = view.get<AimTarget>(entity);

		if (!aimTargetExists(registry, aimTarget))
			aimTarget.entity = player;

		if (!registry.all_of<Position>(aimTarget.entity))
			continue;
		
		Vector3 targetPos = registry.get<Position>(aimTarget.entity).value;
		Vector3 toTarget = targetPos - position.value;
		float dist = Vector3Length(toTarget);

		bulletWeapon.firing = (dist < 100.0f) && Vector3DotProduct(GetForwardVector(rotation), Vector3Normalize(toTarget)) > cosf(DEG2RAD * 20.0f);
	}
}
