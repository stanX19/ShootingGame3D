#include "systems.hpp"
#include "game_context.hpp"
#include "components/physics.hpp"
#include "components/collision.hpp"
#include "components/identity.hpp"
#include "components/camera.hpp"
#include "components/combat.hpp"
#include "utils/vector_rotation_utils.hpp"
#include <cmath>
#include <iostream>

namespace {
	camera::CameraPOV getAimModePOV(GameContext &context, entt::entity entity, camera::CameraPOV defaultPOV) {
		if (!context.registry.valid(entity) && !context.registry.all_of<Position, Rotation>(entity))
			return defaultPOV;
		// closest entity in front of the player
		const Vector3 playerPos = context.registry.get<Position>(entity).value;
		const Quaternion playerRot = context.registry.get<Rotation>(entity).value;
		const Vector3 playerForward = getForwardVector(playerRot);
		
		float closestDist = context.config.ARENA_SIZE * 2;
		float closestAngle = std::cos(1.0f * DEG2RAD);  // minimum 1.0 degrees
		entt::entity closestEntity = entt::null;
		for (auto otherEntity : context.registry.view<Position, tag::Targetable>()) {
			if (otherEntity == entity)
				continue;
			const Position& otherPosComp = context.registry.get<Position>(otherEntity);
			const Vector3 toOther = Vector3Subtract(otherPosComp.value, playerPos);

			const Vector3 toOtherDir = Vector3Normalize(toOther);
			const float dot = Vector3DotProduct(playerForward, toOtherDir);
			if (dot < closestAngle)
				continue;
			closestAngle = dot;

			const float toOtherDist = Vector3Length(toOther);
			closestDist = toOtherDist;
			closestEntity = otherEntity;
		}
		if (closestEntity == entt::null)
			return defaultPOV;
		return camera::CameraPOV{
			.positionOffset = {0.0f, 0.0f, closestDist * 0.9f},
			.targetOffset = {0.0f, 0.0f, closestDist * 1.0f},
			.fovy = 30.0f
		};
	}
}

void systems::CameraFollowPlayer::update(GameContext &context, float dt) {
	if (!context.registry.valid(context.currentPlayer))
		return;

	auto [posPtr, rotPtr, colBodyPtr] = context.registry.try_get<Position, Rotation, CollisionBody>(context.currentPlayer);
	if (!posPtr || !rotPtr)
		return;
	camera::UnitCamera *unitCamera = &m_defaultCamera;
	if (auto cameraComp = context.registry.try_get<camera::UnitCamera>(context.currentPlayer))
		unitCamera = cameraComp;
	const Position& pos = *posPtr;
	const Rotation& rot = *rotPtr;
	Camera3D& camera = context.mainCamera;
	
	float scroll = GetMouseWheelMove();
	if (scroll > 0.0f) {
		unitCamera->isAiming = true;
	} else if (scroll < 0.0f) {
		unitCamera->isAiming = false;
	}
	
	bool lookback = IsKeyDown(KEY_F);
	
	// Select POV based on lookback and aim state
	camera::CameraPOV pov;
	float scale = 1.0f;
	if (lookback) {
		pov = unitCamera->lookBackPOV;
	} else if (unitCamera->isAiming) {
		pov = getAimModePOV(context, context.currentPlayer, unitCamera->aimPOV);
	} else {
		pov = unitCamera->mainPOV;
		if (colBodyPtr) {
			scale = colBodyPtr->radius;
		}
	}

	Vector3 desiredPosition = Vector3RotateByQuaternion(pov.positionOffset, rot.value) * scale + pos.value;
	Vector3 desiredTarget = Vector3RotateByQuaternion(pov.targetOffset, rot.value) * scale + pos.value;
	
	float smoothing = unitCamera->lerpExp;
	float lerp = 1.0f - std::exp(-smoothing * dt);
	Vector3 up = getUpVector(rot);

	camera.position = Vector3Lerp(camera.position, desiredPosition, lerp);
	camera.target = Vector3Lerp(camera.target, desiredTarget, lerp);
	camera.up = Vector3Lerp(camera.up, up, lerp * unitCamera->upLerpFactor);
	
	camera.fovy = pov.fovy;
	camera.projection = CAMERA_PERSPECTIVE;
}

