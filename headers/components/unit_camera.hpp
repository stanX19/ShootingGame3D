#ifndef UNIT_CAMERA_HPP
#define UNIT_CAMERA_HPP
#include "includes.hpp"

// unused for now
namespace camera {
	struct CameraPOV {
		Vector3 positionOffset = {0.0f, 4.0f, -10.0f};		// Offset from the unit's position
		Vector3 targetOffset = {0.0f, 2.0f, 10.0f};			// Offset from the unit's position
		float fovy = 45.0f;									// Standard field of view
	};

	struct UnitCamera {
		CameraPOV mainPOV = CameraPOV{
			.positionOffset = {0.0f, 4.0f, -10.0f},
			.targetOffset = {0.0f, 2.0f, 10.0f},
			.fovy = 45.0f
		};
		CameraPOV aimPOV = CameraPOV{
			.positionOffset = {0.0f, 0.0f, 50.0f},
			.targetOffset = {0.0f, 0.0f, 1000.0f},
			.fovy = 30.0f
		};
		CameraPOV lookBackPOV = CameraPOV{
			.positionOffset = {0.0f, 4.0f, 10.0f},
			.targetOffset = {0.0f, 2.0f, -10.0f},
			.fovy = 90.0f
		};

		// Smoothing / Physics
		float lerpExp = 12.0f;       // Smoothing variable; Higher = higher lerp factor
		float upLerpFactor = 0.2f;   // Up vector lerp multiplier
		bool isAiming = false;        // Whether the camera is in aim mode
	};

	void emplaceUnitCameraBasic(entt::registry &registry, entt::entity entity);
}

#endif // UNIT_CAMERA_HPP