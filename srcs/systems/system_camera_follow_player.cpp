#include "systems.hpp"
#include "utils.hpp"
#include "iostream"
#include "components/unit_camera.hpp"

namespace {
	camera::UnitCamera defaultCamera;

	camera::CameraPOV getAimModePOV(GameContext &context, entt::entity entity, camera::CameraPOV defaultPOV) {
		if (!context.registry.valid(entity) && !context.registry.all_of<Position, Rotation>(entity))
			return defaultPOV;
		// closest entity in front of the player
		Vector3 playerPos = context.registry.get<Position>(entity).value;
		Quaternion playerRot = context.registry.get<Rotation>(entity).value;
		Vector3 playerForward = getForwardVector(playerRot);
		
		float closestDist = context.config.ARENA_SIZE * 2;
		float closestAngle = std::cos(1.0f * DEG2RAD);  // minimum 1.0 degrees
		entt::entity closestEntity = entt::null;
		for (auto otherEntity : context.registry.view<Position, tag::Targetable>()) {
			if (otherEntity == entity)
				continue;
			Position& otherPosComp = context.registry.get<Position>(otherEntity);
			Vector3 toOther = Vector3Subtract(otherPosComp.value, playerPos);

			Vector3 toOtherDir = Vector3Normalize(toOther);
			float dot = Vector3DotProduct(playerForward, toOtherDir);
			if (dot < closestAngle)
				continue;
			closestAngle = dot;

			float toOtherDist = Vector3Length(toOther);
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

void ecs_systems::cameraFollowPlayer(GameContext &context, float dt) {
	
	if (!context.registry.valid(context.currentPlayer))
		return;

	auto [posPtr, rotPtr] = context.registry.try_get<Position, Rotation>(context.currentPlayer);
	if (!posPtr || !rotPtr)
		return;
	camera::UnitCamera *unitCamera = &defaultCamera;
	if (auto cameraComp = context.registry.try_get<camera::UnitCamera>(context.currentPlayer))
		unitCamera = cameraComp;
	Position& pos = *posPtr;
	Rotation& rot = *rotPtr;
	Camera3D& camera = context.mainCamera;
	
	float scroll = GetMouseWheelMove();
	if (scroll > 0.0f) {
		unitCamera->isAiming = true;
	} else if (scroll < 0.0f) {
		unitCamera->isAiming = false;
	}
	
	bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
	
	// Select POV based on shift and aim state
	camera::CameraPOV pov;
	if (shift) {
		pov = unitCamera->lookBackPOV;
	} else if (unitCamera->isAiming) {
		pov = getAimModePOV(context, context.currentPlayer, unitCamera->aimPOV);
	} else {
		pov = unitCamera->mainPOV;
	}

	Vector3 desiredPosition = Vector3RotateByQuaternion(pov.positionOffset, rot.value) + pos.value;
	Vector3 desiredTarget = Vector3RotateByQuaternion(pov.targetOffset, rot.value) + pos.value;
	
	float smoothing = unitCamera->lerpExp;
	float lerp = 1.0f - std::exp(-smoothing * dt);
	Vector3 up = getUpVector(rot);

	camera.position = Vector3Lerp(camera.position, desiredPosition, lerp);
	camera.target = Vector3Lerp(camera.target, desiredTarget, lerp);
	camera.up = Vector3Lerp(camera.up, up, lerp * unitCamera->upLerpFactor);
	
	camera.fovy = pov.fovy;
	camera.projection = CAMERA_PERSPECTIVE;
	// std::cout << "Camera Pos: (" << camera.position.x << ", " 
	// 							   << camera.position.y << ", " 
	// 							   << camera.position.z << ") | Target: (" << camera.target.x << ", " 
	// 							   << camera.target.y << ", " 
	// 							   << camera.target.z << ") | FOV: " << camera.fovy << std::endl;
}
