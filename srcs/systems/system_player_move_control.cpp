#include "systems.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include <iostream>
#include <cmath>

Vector2 getMouseDirectionRelRot(const Quaternion &entRot, const Camera3D &camera) { 
	Vector2 mouseDirection = getMouseDirectionNormalized(0.5f);
		
	if (std::abs(mouseDirection.x) < 0.01f && std::abs(mouseDirection.y) < 0.01f) {
		return {0.0f, 0.0f};
	}

	Vector3 entityUp = GetUpVector(entRot);
	Vector3 entityRight = GetRightVector(entRot);
		
	Vector3 cameraForward = Vector3Normalize(camera.target - camera.position);
	Vector3 cameraRight = Vector3Normalize(Vector3CrossProduct(cameraForward, camera.up));
		
	Vector2 entityFlatUp = Vector2Normalize(Vector2{
		Vector3DotProduct(entityUp, cameraRight),
		-Vector3DotProduct(entityUp, camera.up)
	});
		
	Vector2 entityFlatRight = Vector2Normalize(Vector2{
		Vector3DotProduct(entityRight, cameraRight),
		-Vector3DotProduct(entityRight, camera.up)
	});
		
	return Vector2{
		-Vector2DotProduct(entityFlatRight, mouseDirection),
		-Vector2DotProduct(entityFlatUp, mouseDirection)
	};
}

void ecs_systems::playerMoveControl(GameContext &context, float dt, const Camera3D &camera)
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
	Vector3 fowardVector = GetForwardVector(rotation);
	Vector3 upVector = GetUpVector(rotation);
	Vector3 rightVector = GetRightVector(rotation);

	Vector2 mouseDirection = getMouseDirectionRelRot(rotation.value, camera);
	if (std::abs(mouseDirection.x) >= 0.01f) {
		newRotation = RotateAroundAxis(newRotation, upVector, -mouseDirection.x * turnSpeedDt);
		newRotation = RotateAroundAxis(newRotation, fowardVector, mouseDirection.x * turnSpeedDt * (mouseDirection.y <= -0.0f? 1: -0.2));
	}
	if (std::abs(mouseDirection.y) >= 0.01f) {
		newRotation = RotateAroundAxis(newRotation, rightVector, mouseDirection.y * turnSpeedDt);
	}
	if (IsKeyDown(KEY_RIGHT))
		newRotation = RotateAroundAxis(newRotation, upVector, -turnSpeedDt);
	if (IsKeyDown(KEY_LEFT))
		newRotation = RotateAroundAxis(newRotation, upVector, turnSpeedDt);
	if (IsKeyDown(KEY_UP))
		newRotation = RotateAroundAxis(newRotation, rightVector, -turnSpeedDt);
	if (IsKeyDown(KEY_DOWN))
		newRotation = RotateAroundAxis(newRotation, rightVector, turnSpeedDt);
	if (IsKeyDown(KEY_A))
		newRotation = RotateAroundAxis(newRotation, fowardVector, -turnSpeedDt);
	if (IsKeyDown(KEY_D))
		newRotation = RotateAroundAxis(newRotation, fowardVector, turnSpeedDt);
	
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
