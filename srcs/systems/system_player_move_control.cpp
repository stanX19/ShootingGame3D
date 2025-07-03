#include "systems.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include <iostream>

void ecs_systems::playerMoveControl(GameContext &context, float dt)
{
	if (!context.registry.all_of<Position, Rotation, Velocity, MaxSpeed, TurnSpeed>(context.currentPlayer))
		return ;

	Position &position = context.registry.get<Position>(context.currentPlayer);
	Rotation &rotation = context.registry.get<Rotation>(context.currentPlayer);
	Velocity &velocity = context.registry.get<Velocity>(context.currentPlayer);
	MaxSpeed &maxSpeed = context.registry.get<MaxSpeed>(context.currentPlayer);
	TurnSpeed &turnSpeed = context.registry.get<TurnSpeed>(context.currentPlayer);

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
