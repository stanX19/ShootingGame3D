#include "shoot_3d.hpp"

static void shoot_bullet(entt::registry &registry, Rotation &rotation, Position &position)
{
	Vector3 shootDir = GetForwardVector(rotation);
	Vector3 bestDir = shootDir;
	float bestDot = cosf(DEG2RAD * 30.0f); // 30° cone
	float closestDist = 100.0f;		       // max assist distance

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

	Vector3 spawnPos = Vector3Add(position.value, Vector3Scale(bestDir, 2.0f));
	spawn_bullet(registry, spawnPos, bestDir, BULLET_DAMAGE, BLUE);
}

void ecs_system::player_update(entt::registry &registry, float dt)
{
	auto view = registry.view<Player, Position, Rotation, Velocity, HP>();

	for (auto entity : view)
	{
		Player &player = view.get<Player>(entity);
		Position &position = view.get<Position>(entity);
		Rotation &rotation = view.get<Rotation>(entity);
		Velocity &velocity = view.get<Velocity>(entity);

		player.timeSinceLastShot += dt;

		// Calculate current speed (scalar)
		Vector3 vel = velocity.value;
		float speed = Vector3Length(vel);

		// Turn speed reduces with movement speed
		float turnSpeed = MOUSE_SENSITIVITY / (1.0f + speed / PLAYER_SPEED * 5.0f);
		int uprightFactor = (Vector3DotProduct(GetUpVector(rotation), GetUpVector()) >= 0)? 1: -1;

		if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
			rotation.value.y -= turnSpeed * uprightFactor;
		if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
			rotation.value.y += turnSpeed * uprightFactor;
		if (IsKeyDown(KEY_UP))
			rotation.value.x -= turnSpeed;
		if (IsKeyDown(KEY_DOWN))
			rotation.value.x += turnSpeed;

		rotation.value.x = WrapAngle(rotation.value.x);

		// Adjust speed based on input
		const float accel = 20.0f;
		const float maxSpeed = player.moveSpeed;

		if (IsKeyDown(KEY_W))
		{
			speed += accel * dt;
		}
		else if (IsKeyDown(KEY_S))
		{
			speed -= accel * dt;
		}

		speed = Clamp(speed, 0, maxSpeed);

		// Apply new velocity aligned with current facing
		Vector3 forward = GetForwardVector(rotation);
		velocity.value = Vector3Scale(forward, speed);

		// Shooting
		if ((IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_LEFT_BUTTON)) && player.timeSinceLastShot >= player.shootCooldown)
		{
			shoot_bullet(registry, rotation, position);
			player.timeSinceLastShot = 0;
		}

		// Stay within arena
		position.value.x = Clamp(position.value.x, -ARENA_SIZE, ARENA_SIZE);
		position.value.z = Clamp(position.value.z, -ARENA_SIZE, ARENA_SIZE);
	}
}