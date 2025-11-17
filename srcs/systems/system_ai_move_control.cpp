#include "systems.hpp"
#include "utils.hpp"

static void aiTurnControl(GameContext &context, float dt)
{
	auto view = context.registry.view<Position, Rotation, Velocity, TurnSpeed, MoveTarget>();

	for (auto [entity, position, rotation, velocity, turnSpeed, target] : view.each())
	{
		Vector3 targetPos = {0, 0, 0};
		Vector3 targetVel = {0, 0, 0};
		if (context.registry.valid(target.entity) && context.registry.all_of<Position>(target.entity))
			targetPos = context.registry.get<Position>(target.entity).value;
		if (context.registry.valid(target.entity) && context.registry.all_of<Velocity>(target.entity))
			targetVel = context.registry.get<Velocity>(target.entity).value;

			
		float speed = Vector3Length(velocity.value);
		float calc_speed = speed;
		
		if (context.registry.all_of<ScalarAcceleration>(entity)) {
			float acceleration = context.registry.get<ScalarAcceleration>(entity).value;
			float distance = Vector3Distance(position.value, targetPos);
			float traverlTime = speed + sqrtf(speed * speed + 2 * acceleration * distance) / acceleration;
			calc_speed += acceleration * traverlTime / 2.0f;
		}
		// Vector3 targetDir = targetPos - position.value;
		Vector3 targetDir = calculateLeadDirection(position.value, targetPos, targetVel, calc_speed);
		Quaternion targetRotation = vector3ToRotation(targetDir);
		float turnSpeedDt = turnSpeed.value / (1.0f + speed / 100.0f) * dt;
		float totalTargetTurn = angleDifference(targetRotation, rotation.value) * DEG2RAD;
		rotation.value = QuaternionSlerp(rotation.value, targetRotation, std::min(turnSpeedDt / totalTargetTurn, 1.0f));

		if (context.registry.all_of<Velocity, tag::VelocitySyncRot>(entity))
			context.registry.get<Velocity>(entity).value = getForwardVector(rotation) * speed;
	}
}

static void aiSpeedControl(GameContext &context, float dt)
{
	auto view = context.registry.view<Rotation, Velocity, MaxSpeed, MoveTarget>();

	for (auto [entity, rotation, velocity, maxSpeed, target] : view.each())
	{
		Quaternion targetRotation = rotation.value;
		Vector3 vel = velocity.value;
		float speed = Vector3Length(vel);

		float targetSpeed = maxSpeed.value * (0.5f + 0.5f * (180.0f - angleDifference(targetRotation, rotation.value)) / 180.0f);
		float newSpeed = Clamp(speed + Clamp(targetSpeed - speed, -20.0f * dt, 20.0f * dt), 0.0f, maxSpeed.value);

		velocity.value = getForwardVector(rotation) * newSpeed;
	}
}

void ecs_systems::aiMoveControl(GameContext &context, float dt)
{
	aiTurnControl(context, dt);
	aiSpeedControl(context, dt);
}