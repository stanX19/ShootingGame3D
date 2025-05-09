#include "shoot_3d.hpp"

#define MOUSE_SENSITIVITY 2.5f

void ecs_systems::playerMoveControl(entt::registry &registry, float dt)
{
	auto view = registry.view<Player, Position, Rotation, Velocity, MaxSpeed>();

	for (auto entity : view)
	{
		Position &position = view.get<Position>(entity);
		Rotation &rotation = view.get<Rotation>(entity);
		Velocity &velocity = view.get<Velocity>(entity);
		MaxSpeed &maxSpeed = view.get<MaxSpeed>(entity);

		// Calculate current speed (scalar)
		Vector3 vel = velocity.value;
		float speed = Vector3Length(vel);

		// Turn speed reduces with movement speed
		float turnSpeed = MOUSE_SENSITIVITY / (1.0f + speed / maxSpeed.value * 5.0f);
		Quaternion newRotation = rotation.value;

		if (IsKeyDown(KEY_RIGHT))
			newRotation = RotateAroundAxis(newRotation, GetUpVector(rotation), -turnSpeed * dt);
		if (IsKeyDown(KEY_LEFT))
			newRotation = RotateAroundAxis(newRotation, GetUpVector(rotation), turnSpeed * dt);
		if (IsKeyDown(KEY_UP))
			newRotation = RotateAroundAxis(newRotation, GetRightVector(rotation), turnSpeed * dt);
		if (IsKeyDown(KEY_DOWN))
			newRotation = RotateAroundAxis(newRotation, GetRightVector(rotation), -turnSpeed * dt);
		if (IsKeyDown(KEY_A))
			newRotation = RotateAroundAxis(newRotation, GetForwardVector(rotation), -turnSpeed * dt);
		if (IsKeyDown(KEY_D))
			newRotation = RotateAroundAxis(newRotation, GetForwardVector(rotation), turnSpeed * dt);

		rotation.value = newRotation;

		// Adjust speed based on input
		const float accel = 20.0f;

		if (IsKeyDown(KEY_W))
		{
			speed += accel * dt;
		}
		else if (IsKeyDown(KEY_S))
		{
			speed -= accel * dt;
		}

		speed = Clamp(speed, 0, maxSpeed.value);

		// Apply new velocity aligned with current facing
		velocity.value = Vector3Scale(GetForwardVector(rotation), speed);

		// Stay within arena
		position.value.x = Clamp(position.value.x, -ARENA_SIZE, ARENA_SIZE);
		position.value.z = Clamp(position.value.z, -ARENA_SIZE, ARENA_SIZE);
	}
}
