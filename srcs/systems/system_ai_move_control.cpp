#include "systems.hpp"
#include "utils.hpp"

void ecs_systems::aiMoveControl(GameContext &context, float dt)
{
	auto enemyView = context.registry.view<Position, Rotation, Velocity, MaxSpeed, TurnSpeed, MoveTarget>();

	for (auto [entity, position, rotation, velocity, maxSpeed, turnSpeed, target] : enemyView.each())
	{
		Vector3 targetPos = {0, 0, 0};
		
		if (context.registry.valid(target.entity) && context.registry.all_of<Position>(target.entity))
			targetPos = context.registry.get<Position>(target.entity).value;

		Vector3 toTarget = targetPos - position.value;

		// maybe try to avoid player in the future
		Quaternion targetRotation = vector3ToRotation(toTarget);

		Vector3 vel = velocity.value;
		float speed = Vector3Length(vel);

		float turnSpeedDt = turnSpeed.value / (1.0f + speed / maxSpeed.value * 5.0f) * dt;
		rotation.value = QuaternionSlerp(rotation.value, targetRotation, std::min(turnSpeedDt, 1.0f));

		float targetSpeed = maxSpeed.value * (0.5 + 0.5 * (180 - angleDifference(targetRotation, rotation.value)) / 180);

		float newSpeed = Clamp(speed + Clamp(targetSpeed - speed, -20 * dt, 20 * dt), 0, maxSpeed.value);
		velocity.value = getForwardVector(rotation) * newSpeed;
	}
}