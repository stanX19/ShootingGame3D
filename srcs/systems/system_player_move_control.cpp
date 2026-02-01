#include "systems.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "components/unit_camera.hpp"
#include <iostream>
#include <cmath>

namespace {
	Vector2 getMouseDirectionRelRot(const Quaternion &entRot, const Camera3D &camera) { 
		Vector2 mouseDirection = getMouseDirectionNormalized(0.5f);
			
		if (std::abs(mouseDirection.x) < 0.01f && std::abs(mouseDirection.y) < 0.01f) {
			return {0.0f, 0.0f};
		}

		Vector3 entityUp = getUpVector(entRot);
		Vector3 entityRight = getRightVector(entRot);
			
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

	void applySoftBoundary(Position &position, Velocity &velocity, [[maybe_unused]] float dt) {
		const float softBoundaryStart = ARENA_SIZE * 0.99f;
		const float hardBoundary = ARENA_SIZE;
		
		Vector3 pos = position.value;
		Vector3 vel = velocity.value;
		Vector3 totalBoundaryForce = {0.0f, 0.0f, 0.0f};
		
		// Check each axis for boundary proximity
		Vector3 axes[3] = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
		float positions[3] = {pos.x, pos.y, pos.z};
		
		for (int i = 0; i < 3; i++) {
			float currentPos = positions[i];
			float absCurrentPos = std::abs(currentPos);
			
			if (absCurrentPos < softBoundaryStart)
				continue ;
				
			Vector3 surfaceNormal = axes[i] * (currentPos > 0? -1.0f : 1.0f);
			float velocityTowardSurface = Vector3DotProduct(vel, surfaceNormal * -1.0f);
			
			if (velocityTowardSurface <= 0)
				continue ;
				
			float excess = absCurrentPos - softBoundaryStart;
			float maxExcess = hardBoundary - softBoundaryStart;
			float distanceRatio = excess / maxExcess;

			float forceStrength = distanceRatio * velocityTowardSurface;

			totalBoundaryForce += surfaceNormal * forceStrength;
		}
		
		velocity.value += totalBoundaryForce;
		
		position.value.x = Clamp(position.value.x, -hardBoundary, hardBoundary);
		position.value.y = Clamp(position.value.y, -hardBoundary, hardBoundary);
		position.value.z = Clamp(position.value.z, -hardBoundary, hardBoundary);
	}

	float calculateTurnSpeed(GameContext &context, float speed, TurnSpeed &turnSpeed, float dt) {
		float turnSpeedDt = turnSpeed.value / (1.0f + speed / 100.0f) * dt;
		camera::UnitCamera *unitCamera = context.registry.try_get<camera::UnitCamera>(context.currentPlayer);
		if (unitCamera && unitCamera->isAiming) {
			turnSpeedDt *= 0.075f;
		}
		return turnSpeedDt;
	}
}

// void ecs_systems::playerMoveControl(GameContext &context, float dt, const Camera3D &camera)
// {
// 	if (!context.registry.all_of<Position, Rotation, Velocity, MaxSpeed, TurnSpeed>(context.currentPlayer))
// 		return ;

// 	Position &position = context.registry.get<Position>(context.currentPlayer);
// 	Rotation &rotation = context.registry.get<Rotation>(context.currentPlayer);
// 	Velocity &velocity = context.registry.get<Velocity>(context.currentPlayer);
// 	MaxSpeed &maxSpeed = context.registry.get<MaxSpeed>(context.currentPlayer);
// 	TurnSpeed &turnSpeed = context.registry.get<TurnSpeed>(context.currentPlayer);

// 	Vector3 vel = velocity.value;
// 	// float speed = Vector3Length(vel);  // current speed

// 	// float turnSpeedDt = turnSpeed.value / (1.0f + speed / maxSpeed.value * 5.0f)  * dt;
// 	float turnSpeedDt = turnSpeed.value / 2 * dt;
// 	Quaternion newRotation = rotation.value;
// 	Vector3 fowardVector = getForwardVector(rotation);
// 	Vector3 upVector = getUpVector(rotation);
// 	Vector3 rightVector = getRightVector(rotation);

// 	Vector2 mouseDirection = getMouseDirectionRelRot(rotation.value, camera);
// 	if (std::abs(mouseDirection.x) >= 0.01f) {
// 		newRotation = rotateAroundAxis(newRotation, upVector, -mouseDirection.x * turnSpeedDt);
// 		newRotation = rotateAroundAxis(newRotation, fowardVector, mouseDirection.x * turnSpeedDt * (mouseDirection.y <= -0.0f? 1: 0.2));
// 	}
// 	if (std::abs(mouseDirection.y) >= 0.01f) {
// 		newRotation = rotateAroundAxis(newRotation, rightVector, mouseDirection.y * turnSpeedDt);
// 	}
// 	if (IsKeyDown(KEY_RIGHT))
// 		newRotation = rotateAroundAxis(newRotation, upVector, -turnSpeedDt);
// 	if (IsKeyDown(KEY_LEFT))
// 		newRotation = rotateAroundAxis(newRotation, upVector, turnSpeedDt);
// 	if (IsKeyDown(KEY_UP))
// 		newRotation = rotateAroundAxis(newRotation, rightVector, -turnSpeedDt);
// 	if (IsKeyDown(KEY_DOWN))
// 		newRotation = rotateAroundAxis(newRotation, rightVector, turnSpeedDt);
// 	if (IsKeyDown(KEY_A))
// 		newRotation = rotateAroundAxis(newRotation, fowardVector, -turnSpeedDt);
// 	if (IsKeyDown(KEY_D))
// 		newRotation = rotateAroundAxis(newRotation, fowardVector, turnSpeedDt);
	
// 	// TODO: change to engine thrust component
// 	const float accel = 40.0f;
// 	Vector3 fowardVelocity = fowardVector * Vector3DotProduct(vel, fowardVector);
// 	Vector3 perpendictlarVelocity = vel - fowardVelocity;

// 	// perpendictlarVelocity -= Vector3Normalize(perpendictlarVelocity) * std::min(Vector3Length(perpendictlarVelocity), accel * 0.5f * dt);
// 	if (IsKeyDown(KEY_W) || IsMouseButtonDown(MOUSE_BUTTON_EXTRA) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
// 	{
// 		perpendictlarVelocity = Vector3Zeros;
// 		if (Vector3DotProduct(vel, fowardVector) < 0)
// 			fowardVelocity = Vector3Zeros;
// 		fowardVelocity += fowardVector * accel * dt;
// 	}
// 	else if (IsKeyDown(KEY_S) || IsMouseButtonDown(MOUSE_BUTTON_SIDE) || Vector3Length(fowardVelocity) > maxSpeed.value)
// 	{
// 		fowardVelocity -= fowardVector * accel * dt;
// 	}

// 	// speed = Clamp(speed, 0, maxSpeed.value);

// 	velocity.value = fowardVelocity + perpendictlarVelocity;
// 	rotation.value = newRotation;

// 	// Stay within arena
// 	applySoftBoundary(position, velocity, dt);
// 	entt::entity entity = context.registry.create();

// 	// trail particles
// 	context.registry.emplace<Position>(entity, position);
// 	context.registry.emplace<RenderBody>(entity, context.modelManager.createSphere(), ColorAlpha(SKYBLUE, 0.5), 0.25f);
// 	context.registry.emplace<RadiusExpand>(entity, -0.25f);
// 	context.registry.emplace<Lifespan>(entity, 1.0f);
// }


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

	float turnSpeedDt = calculateTurnSpeed(context, speed, turnSpeed, dt);
	Quaternion newRotation = rotation.value;
	Vector3 fowardVector = getForwardVector(rotation);
	Vector3 upVector = getUpVector(rotation);
	Vector3 rightVector = getRightVector(rotation);

	Vector2 mouseDirection = getMouseDirectionRelRot(rotation.value, context.mainCamera);
	if (std::abs(mouseDirection.x) >= 0.01f) {
		newRotation = rotateAroundAxis(newRotation, upVector, -mouseDirection.x * turnSpeedDt);
		newRotation = rotateAroundAxis(newRotation, fowardVector, mouseDirection.x * turnSpeedDt * (mouseDirection.y <= -0.0f? 1: 0.2));
	}
	if (std::abs(mouseDirection.y) >= 0.01f) {
		newRotation = rotateAroundAxis(newRotation, rightVector, mouseDirection.y * turnSpeedDt);
	}
	if (IsKeyDown(KEY_RIGHT))
		newRotation = rotateAroundAxis(newRotation, upVector, -turnSpeedDt);
	if (IsKeyDown(KEY_LEFT))
		newRotation = rotateAroundAxis(newRotation, upVector, turnSpeedDt);
	if (IsKeyDown(KEY_UP))
		newRotation = rotateAroundAxis(newRotation, rightVector, -turnSpeedDt);
	if (IsKeyDown(KEY_DOWN))
		newRotation = rotateAroundAxis(newRotation, rightVector, turnSpeedDt);
	if (IsKeyDown(KEY_A))
		newRotation = rotateAroundAxis(newRotation, fowardVector, -turnSpeedDt);
	if (IsKeyDown(KEY_D))
		newRotation = rotateAroundAxis(newRotation, fowardVector, turnSpeedDt);
	
	// TODO: change to engine thrust component
	static float boostCooldown = 0;
	static float boostDuration = 0;
	float accel = 40.0f;
	if ((IsKeyDown(KEY_E) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
	&& boostCooldown <= 0) {
		boostDuration = 0.5f;
		boostCooldown = 6.0f;
		context.soundManager.playImmediate(context.config, "sounds.hyperBoost");
	}
	if (boostCooldown > 0)
		boostCooldown -= dt;
	if (boostDuration > 0) {
		boostDuration -= dt;
		accel *= 15.0f;
	}

	if ((
		(IsKeyDown(KEY_W) || IsMouseButtonDown(MOUSE_BUTTON_EXTRA)
			|| IsMouseButtonDown(MOUSE_BUTTON_RIGHT)))// && speed < maxSpeed.value)
		|| boostDuration > 0)
	{
		speed += accel * dt;
	}
	else if (IsKeyDown(KEY_S) || IsMouseButtonDown(MOUSE_BUTTON_SIDE) || speed > maxSpeed.value)
	{
		speed -= accel * dt;
	}

	// speed = Clamp(speed, 0, maxSpeed.value);

	velocity.value = Vector3Scale(getForwardVector(rotation), speed);
	rotation.value = newRotation;

	// Stay within arena
	applySoftBoundary(position, velocity, dt);
}
