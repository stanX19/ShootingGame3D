#ifndef ENTITIES_TURRET_HPP
#define ENTITIES_TURRET_HPP

#include "game_context.hpp"

namespace turret
{

	enum class TurretControlMode
	{
		FollowParent,
		Autonomous
	};

	entt::entity spawnConfiguredTurret(
		GameContext &context,
		Color color,
		entt::entity parent,
		Vector3 relativePosition,
		Quaternion relativeRotation,
		float radius,
		TurretControlMode controlMode
	);

	entt::entity spawnUnlinkedAutoTurret(
		GameContext &context, Color color
	);
	entt::entity spawnLinkedTurret(
		GameContext &context,
		Color color,
		entt::entity &parent,
		Vector3 relativePosition
	);
	entt::entity spawnLinkedAutoTurret(
		GameContext &context,
		Color color,
		entt::entity &parent,
		Vector3 relativePosition
	);

} // namespace turret

#endif // ENTITIES_TURRET_HPP
