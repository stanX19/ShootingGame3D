#include "entities/turret.hpp"
#include "components/faction.hpp"
#include "components/physics.hpp"
#include "components/anchor.hpp"
#include "components/combat.hpp"
#include "components/collision.hpp"
#include "components/render.hpp"
#include "components/weapon.hpp"
#include "components/score.hpp"
#include "components/effect.hpp"

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
		context.registry.emplace<physics::Position>(turret);
		context.registry.emplace<physics::Rotation>(turret);
		context.registry.emplace<collision::CollisionBody>(turret, radius);
		context.registry.emplace<render::RenderBody>(
			turret, render::RenderBody{turretModel, color, radius}
		);
		context.registry.emplace<combat::HP>(turret, 750.0f);
		context.registry.emplace<combat::HPRegen>(turret, 10.0f);
		context.registry.emplace<render::tag::Shaded>(turret);
		context.registry.emplace<render::tag::AimDirectionSyncModel>(turret);
		context.registry.emplace<effect::tag::DropDebris>(turret);
		context.registry.emplace<effect::ExplodeOnDeath>(
			turret,
			effect::ExplodeOnDeath::createFromRadDmg(
				radius,
				effect::DEFAULT_EXPLOSION_DAMAGE
			)
		);
		context.registry.emplace<physics::Mass>(
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
		context.registry.emplace_or_replace<anchor::PositionAnchor>(
			turret,
			anchor::PositionAnchor{parent, relativePosition}
		);
		context.registry.emplace_or_replace<anchor::RotationAnchor>(
			turret,
			anchor::RotationAnchor{parent}
		);
		context.registry.emplace_or_replace<weapon::WeaponParent>(
			turret,
			weapon::WeaponParent{parent}
		);
		context.registry.emplace_or_replace<anchor::DeathAnchor>(
			turret,
			anchor::DeathAnchor{parent, 0.75f}
		);
		context.registry.emplace_or_replace<score::ScoreParent>(
			turret,
			score::ScoreParent{parent}
		);

		const physics::Position* parentPosition = context.registry.try_get<physics::Position>(parent);
		const physics::Rotation* parentRotation = context.registry.try_get<physics::Rotation>(parent);
		if (parentPosition == nullptr || parentRotation == nullptr)
			return;
		context.registry.emplace_or_replace<physics::Position>(
			turret,
			physics::Position{
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
			context.registry.emplace<weapon::tag::FollowParentAim>(turret);
			context.registry.emplace<weapon::tag::FollowParentFire>(turret);
			return;
		}
		context.registry.emplace<weapon::tag::AIControlledAim>(turret);
		context.registry.emplace<weapon::tag::AIControlledFire>(turret);
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
	context.registry.get<anchor::RotationAnchor>(turretEntity).relrot = relativeRotation;
	addControlTags(context, turretEntity, controlMode);
	return turretEntity;
}

} // namespace turret

entt::entity spawnUnlinkedAutoTurret(GameContext& context, Color color) {
	entt::entity turretEntity = spawnBaseTurret(context, color, 0.25f);
	context.registry.emplace<weapon::tag::AIControlledAim>(turretEntity);
	context.registry.emplace<weapon::tag::AIControlledFire>(turretEntity);
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
