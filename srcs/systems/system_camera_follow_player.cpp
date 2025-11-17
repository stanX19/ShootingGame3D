#include "systems.hpp"
#include "utils.hpp"
#include "iostream"
#include "components/unit_camera.hpp"

void ecs_systems::cameraFollowPlayer(GameContext &context, float dt) {
	
	if (!context.registry.valid(context.currentPlayer))
		return;

	auto [posPtr, rotPtr] = context.registry.try_get<Position, Rotation>(context.currentPlayer);
	if (!posPtr || !rotPtr)
		return;
	camera::UnitCamera unitCamera;
	if (context.registry.all_of<camera::UnitCamera>(context.currentPlayer))
		unitCamera = context.registry.get<camera::UnitCamera>(context.currentPlayer);
	Position& pos = *posPtr;
	Rotation& rot = *rotPtr;
	Camera3D& camera = context.mainCamera;
	
	static bool isAiming = false;
	
	float scroll = GetMouseWheelMove();
	if (scroll > 0.0f) {
		isAiming = true;
	} else if (scroll < 0.0f) {
		isAiming = false;
	}
	
	bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
	
	// Select POV based on shift and aim state
	camera::CameraPOV pov;
	if (shift) {
		pov = unitCamera.lookBackPOV;
	} else {
		pov = isAiming ? unitCamera.aimPOV : unitCamera.mainPOV;
	}

	Vector3 desiredPosition = Vector3RotateByQuaternion(pov.positionOffset, rot.value) + pos.value;
	Vector3 desiredTarget = Vector3RotateByQuaternion(pov.targetOffset, rot.value) + pos.value;
	
	float smoothing = unitCamera.lerpExp;
	float lerp = 1.0f - std::exp(-smoothing * dt);
	Vector3 up = getUpVector(rot);

	camera.position = Vector3Lerp(camera.position, desiredPosition, lerp);
	camera.target = Vector3Lerp(camera.target, desiredTarget, lerp);
	camera.up = Vector3Lerp(camera.up, up, lerp * unitCamera.upLerpFactor);
	
	camera.fovy = pov.fovy;
	camera.projection = CAMERA_PERSPECTIVE;
	// std::cout << "Camera Pos: (" << camera.position.x << ", " 
	// 							   << camera.position.y << ", " 
	// 							   << camera.position.z << ") | Target: (" << camera.target.x << ", " 
	// 							   << camera.target.y << ", " 
	// 							   << camera.target.z << ") | FOV: " << camera.fovy << std::endl;
}
