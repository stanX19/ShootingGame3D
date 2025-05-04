#include "shoot_3d.hpp"

static Vector3 calculate_aim_rotation(entt::registry &registry, Rotation &rotation, Position &position)
{
	Vector3 shootDir = GetForwardVector(rotation);
	Vector3 bestDir = shootDir;
	float bestDot = cosf(DEG2RAD * 30.0f); // 30° cone
	float closestDist = 100.0f;			   // max assist distance

	auto enemyView = registry.view<Enemy, Position, HP>();
	for (auto enemyEntity : enemyView)
	{
		Position &enemyPos = enemyView.get<Position>(enemyEntity);
		HP &enemyHp = enemyView.get<HP>(enemyEntity);
		if (enemyHp.value <= 0)
			continue;

		Vector3 toEnemy = Vector3Subtract(enemyPos.value, position.value);
		float dist = Vector3Length(toEnemy);
		Vector3 dirToEnemy = Vector3Normalize(toEnemy);

		float dot = Vector3DotProduct(shootDir, dirToEnemy);
		if (dot > bestDot && dist < closestDist)
		{
			bestDir = dirToEnemy;
			closestDist = dist;
		}
	}

	return vectorToRotation(bestDir);
}

void ecs_system::player_aim(entt::registry &registry)
{
	auto view = registry.view<Player, Position, Rotation, AimRotation, BulletWeapon>();

	for (auto entity : view)
	{
		Position &position = view.get<Position>(entity);
		Rotation &rotation = view.get<Rotation>(entity);
		AimRotation &aimRotation = view.get<AimRotation>(entity);
		BulletWeapon &bulletWeapon = view.get<BulletWeapon>(entity);

		bulletWeapon.firing = (IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_LEFT_BUTTON));

		if (!bulletWeapon.firing)
			continue;

		aimRotation.value = calculate_aim_rotation(registry, rotation, position);
	}
}
