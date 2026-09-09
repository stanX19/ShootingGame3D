#include "systems.hpp"
#include "game_context.hpp"
#include "components/physics.hpp"
#include "components/movement.hpp"
#include "components/unit.hpp"
#include "utils/math_utils.hpp"
#include "utils/vector_rotation_utils.hpp"
#include "utils/algorithm_utils.hpp"
#include <cmath>

static void aiTurnControl(GameContext &context, [[maybe_unused]] float dt)
{
	auto view = context.registry.view<Position, Rotation, Velocity, TurnSpeed, MoveTarget, tag::AIMoveControl>();

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
		
		Vector3 targetDir = calculateLeadDirection(position.value, targetPos, targetVel, calc_speed);
		float distance = Vector3Distance(position.value, targetPos);
		float relSpeed = Vector3Length(targetVel - velocity.value);

		const float avoidance_time = 1.0f;
		if (!context.registry.all_of<tag::Suicidal>(entity) && distance / relSpeed < avoidance_time) {
			targetDir = Vector3Normalize(Vector3Normalize(position.value - targetPos) + getUpVector(rotation) * 0.9f);
		}

		Quaternion targetRotation = vector3ToRotation(targetDir);
		TargetRotation &tRot = context.registry.get_or_emplace<TargetRotation>(entity);
		tRot.value = targetRotation;
	}
}

static void aiSpeedControl(GameContext &context, float dt)
{
	auto view = context.registry.view<Rotation, Velocity, MaxSpeed, MoveTarget, tag::AIMoveControl>();

	for (auto [entity, rotation, velocity, maxSpeed, target] : view.each())
	{
		Quaternion targetRotation = rotation.value;
		Vector3 vel = velocity.value;
		float speed = Vector3Length(vel);

		float targetSpeed = maxSpeed.value * (0.5f + 0.5f * (180.0f - angleDifference(targetRotation, rotation.value)) / 180.0f);
		float newSpeed = Clamp(speed + Clamp(targetSpeed - speed, -20.0f * dt, 20.0f * dt), 0.0f, maxSpeed.value);

		TargetVelocity &tVel = context.registry.get_or_emplace<TargetVelocity>(entity);
		tVel.value = getForwardVector(rotation) * newSpeed;
	}
}

void systems::AiMoveControl::update(GameContext &context, float dt)
{
	aiTurnControl(context, dt);
	aiSpeedControl(context, dt);
}