#include "entities/spaceship_factory.hpp"

#include <cmath>
#include <stdexcept>

#include "components/faction.hpp"
#include "components/physics.hpp"
#include "components/spaceship.hpp"
#include "components/combat.hpp"
#include "components/collision.hpp"
#include "components/render.hpp"
#include "components/effect.hpp"
#include "components/identity.hpp"
#include "components/score.hpp"
#include "utils.hpp"
#include <iostream>

namespace spaceship::factory {


entt::entity SpawnedSpaceship::turret(std::size_t index) const {
	if (index >= turrets.size())
		throw std::out_of_range("SPACESHIP: turret index out of range");
	return turrets[index];
}

ModelAndMounts getModelAndMounts(
	const GameConfig& config,
	ModelManager& modelManager,
	std::string_view shipId,
	float radius
) {
	if (!std::isfinite(radius) || radius <= 0.0f)
		throw std::invalid_argument("SPACESHIP: radius must be positive");
	CollisionBodyManager dummyManager;
	return getModelAndMounts(config, modelManager, dummyManager, shipId, radius);
}

ModelAndMounts getModelAndMounts(
	const GameConfig& config,
	ModelManager& modelManager,
	CollisionBodyManager& collisionBodyManager,
	std::string_view shipId,
	float radius
) {
	if (!std::isfinite(radius) || radius <= 0.0f)
		throw std::invalid_argument("SPACESHIP: radius must be positive");
	const auto& definition = config.spaceship().get(shipId);
	const float bodyScale = radius;

	ModelAndMounts result;
	result.modelId = modelManager.loadModel(definition.modelPath);
	result.bodyScale = bodyScale;
	if (!definition.collisionModelPath.empty())
		result.collisionModelId = collisionBodyManager.loadCollisionModel(definition.collisionModelPath);
	result.modelRadius = definition.modelRadius;
	result.engines = definition.engines;
	result.mounts = definition.mounts;

	for (auto& engine : result.engines) {
		engine.center = engine.center * bodyScale;
		engine.radius *= bodyScale;
		engine.length *= bodyScale;
		engine.nozzleDepth *= bodyScale;
	}
	for (auto& mount : result.mounts) {
		mount.position = mount.position * bodyScale;
		mount.supportRoot = mount.supportRoot * bodyScale;
		mount.turretRadius *= bodyScale;
		mount.barrelRadius *= bodyScale;
		mount.barrelLength *= bodyScale;
		mount.supportWidth *= bodyScale;
		mount.supportHeight *= bodyScale;
		mount.socketHeight *= bodyScale;
	}
	return result;
}

SpawnedSpaceship spawnConfiguredSpaceship(
	GameContext& context,
	std::string_view shipId,
	const SpawnParams& params
) {
	const ModelAndMounts geometry = getModelAndMounts(
		context.config,
		context.modelManager,
		context.collisionBodyManager,
		shipId,
		params.radius
	);

	SpawnedSpaceship assembly;
	assembly.entity = context.registry.create();

	context.registry.emplace<physics::Position>(assembly.entity, params.position);
	context.registry.emplace<physics::Velocity>(assembly.entity);
	context.registry.emplace<physics::Rotation>(assembly.entity, params.rotation);
	context.registry.emplace<collision::CollisionBody>(assembly.entity, params.radius);
	if (geometry.collisionModelId.has_value())
		context.registry.emplace<collision::CollisionBodyModel>(assembly.entity, *geometry.collisionModelId);
	context.registry.emplace<collision::Assembly>(assembly.entity, assembly.entity);
	context.registry.emplace<identity::Owner>(assembly.entity, assembly.entity);
	context.registry.emplace<render::RenderBody>(assembly.entity, render::RenderBody{geometry.modelId, params.bodyColor, geometry.bodyScale, Vector3Zeros, params.rotation});
	context.registry.emplace<faction::Faction>(assembly.entity, faction::Faction{params.faction});
	context.registry.emplace<combat::tag::Targetable>(assembly.entity);
	context.registry.emplace<identity::tag::Spaceship>(assembly.entity);
	context.registry.emplace<render::tag::Shaded>(assembly.entity);
	context.registry.emplace<render::tag::RotationSyncModel>(assembly.entity);
	context.registry.emplace<effect::tag::DropDebris>(assembly.entity);
	effect::HasMultiTrail shipTrail;
	shipTrail.maxNodes = 10;
	shipTrail.maxAge = 0.35f;
	shipTrail.minDistance = 1.5f;
	shipTrail.endWidth = 0.0f;
	shipTrail.color = SKYBLUE;
	shipTrail.emitterCount = static_cast<std::uint8_t>(std::min(geometry.engines.size(), effect::HasMultiTrail::MAX_EMITTERS));
	for (std::size_t i = 0; i < shipTrail.emitterCount; ++i) {
		const auto &engine = geometry.engines[i];
		shipTrail.emitters[i].localOffset = Vector3{
			engine.center.x,
			engine.center.y,
			engine.center.z - engine.length * 0.5f - engine.nozzleDepth
		};
		shipTrail.emitters[i].width = std::max(0.15f, engine.radius * 2.0f);
	}
	context.registry.emplace<effect::HasMultiTrail>(assembly.entity, shipTrail);

	assembly.turrets.reserve(geometry.mounts.size());

	for (const auto& mount : geometry.mounts) {
		const entt::entity turretEntity = turret::spawnConfiguredTurret(
			context,
			params.turretColor,
			assembly.entity,
			mount.position,
			vector3ToRotation(mount.forward),
			mount.turretRadius,
			params.turretControl
		);
		assembly.turrets.push_back(turretEntity);
	}
	return assembly;
}

} // namespace spaceship::factory
