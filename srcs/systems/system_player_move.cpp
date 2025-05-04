#include "shoot_3d.hpp"

void ecs_system::player_move(entt::registry &registry, float dt)
{
	auto view = registry.view<Player, Position, Rotation, Velocity>();

	for (auto entity : view)
	{
		Player &player = view.get<Player>(entity);
		Position &position = view.get<Position>(entity);
		Rotation &rotation = view.get<Rotation>(entity);
		Velocity &velocity = view.get<Velocity>(entity);

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

		// Stay within arena
		position.value.x = Clamp(position.value.x, -ARENA_SIZE, ARENA_SIZE);
		position.value.z = Clamp(position.value.z, -ARENA_SIZE, ARENA_SIZE);
	}
}
