#include "systems.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include <iostream>

void ecs_systems::playerMoveControl(GameContext &context, float dt)
{
	auto view = context.registry.view<tag::Player, Position, Rotation, Velocity, MaxSpeed, TurnSpeed>();

	for (auto entity : view)
	{
		Position &position = view.get<Position>(entity);
		Rotation &rotation = view.get<Rotation>(entity);
		Velocity &velocity = view.get<Velocity>(entity);
		MaxSpeed &maxSpeed = view.get<MaxSpeed>(entity);
		TurnSpeed &turnSpeed = view.get<TurnSpeed>(entity);

		Vector3 vel = velocity.value;
		float speed = Vector3Length(vel);  // current speed

		float turnSpeedDt = turnSpeed.value / (1.0f + speed / maxSpeed.value * 5.0f)  * dt;
		Quaternion newRotation = rotation.value;

		Vector2 mouseDirection = getMouseDirectionNormalized(0.5);
		if (std::abs(mouseDirection.x) >= 0.01)
			newRotation = RotateAroundAxis(newRotation, GetUpVector(rotation), -mouseDirection.x * turnSpeedDt);
		if (std::abs(mouseDirection.y) >= 0.01)
			newRotation = RotateAroundAxis(newRotation, GetRightVector(rotation), mouseDirection.y * turnSpeedDt);
		if (IsKeyDown(KEY_RIGHT))
			newRotation = RotateAroundAxis(newRotation, GetUpVector(rotation), -turnSpeedDt);
		if (IsKeyDown(KEY_LEFT))
			newRotation = RotateAroundAxis(newRotation, GetUpVector(rotation), turnSpeedDt);
		if (IsKeyDown(KEY_UP))
			newRotation = RotateAroundAxis(newRotation, GetRightVector(rotation), -turnSpeedDt);
		if (IsKeyDown(KEY_DOWN))
			newRotation = RotateAroundAxis(newRotation, GetRightVector(rotation), turnSpeedDt);
		if (IsKeyDown(KEY_A))
			newRotation = RotateAroundAxis(newRotation, GetForwardVector(rotation), -turnSpeedDt);
		if (IsKeyDown(KEY_D))
			newRotation = RotateAroundAxis(newRotation, GetForwardVector(rotation), turnSpeedDt);
		
		// TODO: change to engine thrust component
		const float accel = 20.0f;

		if (IsKeyDown(KEY_W) || IsMouseButtonDown(MOUSE_BUTTON_EXTRA) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
		{
			speed += accel * dt;
		}
		else if (IsKeyDown(KEY_S) || IsMouseButtonDown(MOUSE_BUTTON_SIDE) || speed > maxSpeed.value / 2)
		{
			speed -= accel * dt;
		}

		speed = Clamp(speed, 0, maxSpeed.value);

		velocity.value = Vector3Scale(GetForwardVector(rotation), speed);
		rotation.value = newRotation;

		// Stay within arena
		position.value.x = Clamp(position.value.x, -ARENA_SIZE, ARENA_SIZE);
		position.value.z = Clamp(position.value.z, -ARENA_SIZE, ARENA_SIZE);
	}
}
