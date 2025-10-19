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

		Vector3 targetDir = calculateLeadDirection(position.value, targetPos, targetVel, Vector3Length(velocity.value));
		// Vector3 targetDir = targetPos - position.value;
		Quaternion targetRotation = vector3ToRotation(targetDir);

		float speed = Vector3Length(velocity.value);
		float turnSpeedDt = turnSpeed.value / (1.0f + speed / 100.0f) * dt;

		rotation.value = QuaternionSlerp(rotation.value, targetRotation, std::min(turnSpeedDt, 1.0f));

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