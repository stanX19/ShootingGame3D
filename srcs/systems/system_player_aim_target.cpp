#include "systems.hpp"
#include "utils.hpp"
#include "constants.hpp"

static entt::entity calculateAimTarget(GameContext &context, Rotation &rotation, Position &position)
{
	Vector3 fowardDir = getForwardVector(rotation);
	Vector3 bestDir = fowardDir;
	float bestDot = cosf(DEG2RAD * 20.0f); // 20° cone
	float minDist = COMBAT_DIST;			   // max assist distance
	entt::entity bestTarget = entt::null;

	auto enemyView = context.registry.view<tag::Enemy, Position, HP>();
	for (auto enemyEntity : enemyView)
	{
		Position &enemyPos = enemyView.get<Position>(enemyEntity);
		HP &enemyHp = enemyView.get<HP>(enemyEntity);
		if (enemyHp.value <= 0)
			continue;

		Vector3 toEnemy = Vector3Subtract(enemyPos.value, position.value - fowardDir * 2.5f);
		float dist = Vector3Length(toEnemy);
		Vector3 dirToEnemy = Vector3Normalize(toEnemy);

		if (dist > minDist)
			continue;

		float dot = Vector3DotProduct(fowardDir, dirToEnemy);
		if (dot > bestDot)
		{
			bestDir = dirToEnemy;
			bestTarget = enemyEntity;
		}
	}
	return bestTarget;
}

void ecs_systems::playerAimTarget(GameContext &context)
{
	auto view = context.registry.view<tag::Player, Position, Rotation, AimTarget>();

	for (auto entity : view)
	{
		Position &position = view.get<Position>(entity);
		Rotation &rotation = view.get<Rotation>(entity);
		AimTarget &aimTarget = view.get<AimTarget>(entity);

		aimTarget.entity = calculateAimTarget(context, rotation, position);
	}
}


