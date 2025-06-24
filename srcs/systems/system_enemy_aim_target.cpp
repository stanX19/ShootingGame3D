#include "shoot_3d.hpp"

void ecs_systems::enemyAimTarget(GameContext &context)
{
	auto playerView = context.registry.view<tag::Player, Position>();
	if (playerView.begin() == playerView.end())
		return;

	entt::entity player = *playerView.begin();

	auto view = context.registry.view<tag::Enemy, Position, Rotation, BulletWeapon, AimTarget>();

	for (auto entity : view)
	{
		Position &position = view.get<Position>(entity);
		Rotation &rotation = view.get<Rotation>(entity);
		BulletWeapon &bulletWeapon = view.get<BulletWeapon>(entity);
		AimTarget &aimTarget = view.get<AimTarget>(entity);

		if (!aimTargetExists(context, aimTarget))
			aimTarget.entity = player;

		if (!context.registry.all_of<Position>(aimTarget.entity))
			continue;
		
		Vector3 targetPos = context.registry.get<Position>(aimTarget.entity).value;
		Vector3 toTarget = targetPos - position.value;
		float dist = Vector3Length(toTarget);

		bulletWeapon.firing = (dist < 100.0f) && Vector3DotProduct(GetForwardVector(rotation), Vector3Normalize(toTarget)) > cosf(DEG2RAD * 20.0f);
	}
}
