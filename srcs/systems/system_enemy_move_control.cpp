#include "shoot_3d.hpp"

void ecs_systems::enemyMoveControl(entt::registry &registry, float dt)
{
	auto playerView = registry.view<Player, Position>();
	if (playerView.begin() == playerView.end())
		return;

	// Get the first player's position (assuming single-player)
	Position playerPos = playerView.get<Position>(*playerView.begin());

	auto enemyView = registry.view<Enemy, Position, Rotation, Velocity, MaxSpeed, TurnSpeed>();

	for (auto entity : enemyView)
	{
		Position &position = enemyView.get<Position>(entity);
		Rotation &rotation = enemyView.get<Rotation>(entity);
		Velocity &velocity = enemyView.get<Velocity>(entity);
		MaxSpeed &maxSpeed = enemyView.get<MaxSpeed>(entity);
		TurnSpeed &turnSpeed = enemyView.get<TurnSpeed>(entity);

		Vector3 toPlayer = playerPos.value - position.value;
		float dist = Vector3Length(toPlayer);

		Quaternion targetRotation = vector3ToRotation(toPlayer);

		// Calculate current speed (scalar)
		Vector3 vel = velocity.value;
		float speed = Vector3Length(vel);

		// Turn speed reduces with movement speed (same as player)
		float turnSpeedDt = turnSpeed.value / (1.0f + speed / maxSpeed.value * 5.0f) * dt;
		rotation.value = QuaternionSlerp(rotation.value, targetRotation, std::min(turnSpeedDt, 1.0f));

		float targetSpeed = 0;
		if (dist > 10.0f)
		{
			targetSpeed = maxSpeed.value * (180 - angleDifference(targetRotation, rotation.value)) / 180;
		}
		// else targetSpeed = 0
		float newSpeed = Clamp(speed + Clamp(targetSpeed - speed, -20 * dt, 20 * dt), 0, maxSpeed.value);
		velocity.value = GetForwardVector(rotation) * newSpeed; // Move at new speed
	}
}