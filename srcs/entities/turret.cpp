#include "entities/turret.hpp"
#include "components/factions.hpp"
#include "components.hpp"

#include <stdexcept>

namespace {
	entt::entity spawnBaseTurret(
		GameContext& context,
		Color color,
		float radius
	) {
		entt::entity turret = context.registry.create();
		const t_model_id turretModel =
			context.modelManager.loadModel("assets/Models/canon/canon3.glb");
		context.registry.emplace<Position>(turret);
		context.registry.emplace<Rotation>(turret);
		context.registry.emplace<CollisionBody>(turret, radius);
		context.registry.emplace<RenderBody>(
			turret, RenderBody{turretModel, color, radius}
		);
		context.registry.emplace<HP>(turret, 750.0f);
		context.registry.emplace<HPRegen>(turret, 10.0f);
		context.registry.emplace<tag::Shaded>(turret);
		context.registry.emplace<tag::AimDirectionSyncModel>(turret);
		context.registry.emplace<tag::effect::DropDebris>(turret);
		context.registry.emplace<effect::ExplodeOnDeath>(
			turret,
			effect::ExplodeOnDeath::createFromRadDmg(
				radius,
				effect::DEFAULT_EXPLOSION_DAMAGE
			)
		);
		context.registry.emplace<Mass>(
			turret,
			context.config.getFloat("units.turret.mass", 500.0f)
		);
		return turret;
	}

	void linkWithParent(
		GameContext& context,
		entt::entity turret,
		entt::entity parent,
		Vector3 relativePosition
	) {
		context.registry.emplace_or_replace<PositionAnchor>(
			turret,
			PositionAnchor{parent, relativePosition}
		);
		context.registry.emplace_or_replace<RotationAnchor>(
			turret,
			RotationAnchor{parent}
		);
		context.registry.emplace_or_replace<WeaponParent>(
			turret,
			WeaponParent{parent}
		);
		context.registry.emplace_or_replace<DeathAnchor>(
			turret,
			DeathAnchor{parent, 0.75f}
		);
		context.registry.emplace_or_replace<ScoreParent>(
			turret,
			ScoreParent{parent}
		);

		const Position* parentPosition = context.registry.try_get<Position>(parent);
		const Rotation* parentRotation = context.registry.try_get<Rotation>(parent);
		if (parentPosition == nullptr || parentRotation == nullptr)
			return;
		context.registry.emplace_or_replace<Position>(
			turret,
			Position{
				parentPosition->value
					+ Vector3RotateByQuaternion(
						relativePosition,
						parentRotation->value
					)
			}
		);

		const faction::Faction* parentFaction =
			context.registry.try_get<faction::Faction>(parent);
		if (parentFaction != nullptr)
			context.registry.emplace_or_replace<faction::Faction>(
				turret,
				faction::Faction{parentFaction->value}
			);
	}

	void addControlTags(
		GameContext& context,
		entt::entity turret,
		turret::TurretControlMode controlMode
	) {
		if (controlMode == turret::TurretControlMode::FollowParent) {
			context.registry.emplace<tag::weapon::FollowParentAim>(turret);
			context.registry.emplace<tag::weapon::FollowParentFire>(turret);
			return;
		}
		context.registry.emplace<tag::weapon::AIControlledAim>(turret);
		context.registry.emplace<tag::weapon::AIControlledFire>(turret);
	}
}

namespace turret {

entt::entity spawnConfiguredTurret(
	GameContext& context,
	Color color,
	entt::entity parent,
	Vector3 relativePosition,
	Quaternion relativeRotation,
	float radius,
	TurretControlMode controlMode
) {
	if (radius <= 0.0f)
		throw std::invalid_argument("TURRET: radius must be positive");
	entt::entity turretEntity = spawnBaseTurret(context, color, radius);
	linkWithParent(context, turretEntity, parent, relativePosition);
	context.registry.get<RotationAnchor>(turretEntity).relrot = relativeRotation;
	addControlTags(context, turretEntity, controlMode);
	return turretEntity;
}

} // namespace turret

entt::entity spawnUnlinkedAutoTurret(GameContext& context, Color color) {
	entt::entity turretEntity = spawnBaseTurret(context, color, 0.25f);
	context.registry.emplace<tag::weapon::AIControlledAim>(turretEntity);
	context.registry.emplace<tag::weapon::AIControlledFire>(turretEntity);
	return turretEntity;
}

entt::entity spawnLinkedTurret(
	GameContext& context,
	Color color,
	entt::entity& parent,
	Vector3 relativePosition
) {
	return turret::spawnConfiguredTurret(
		context,
		color,
		parent,
		relativePosition,
		QuaternionUnitX,
		0.25f,
		turret::TurretControlMode::FollowParent
	);
}

entt::entity spawnLinkedAutoTurret(
	GameContext& context,
	Color color,
	entt::entity& parent,
	Vector3 relativePosition
) {
	return turret::spawnConfiguredTurret(
		context,
		color,
		parent,
		relativePosition,
		QuaternionUnitX,
		0.25f,
		turret::TurretControlMode::Autonomous
	);
}
