#include "entities/spaceship_factory.hpp"

#include <cmath>
#include <stdexcept>

#include "components/factions.hpp"
#include "components.hpp"
#include "utils.hpp"
#include <iostream>

namespace spaceship::factory {

namespace {

SpawnsTrailParticles makeShipTrailParticles(
	const std::vector<EnginePoint>& engines,
	float bodyScale
) {
	if (engines.size() > SpawnsTrailParticles::maxSpawnLocations)
		throw std::invalid_argument(
			"SPACESHIP: engine count exceeds trail spawn capacity"
		);

	SpawnsTrailParticles result{};
	result.spawnCount = static_cast<std::uint8_t>(engines.size());
	result.radius = 0.3f * bodyScale;
	result.lifespan = 0.1f;
	result.color = SKYBLUE;
	for (std::size_t index = 0; index < engines.size(); ++index) {
		const EnginePoint& engine = engines[index];
		result.spawnLocations[index] = Vector3{
			engine.center.x,
			engine.center.y,
			engine.center.z
				- engine.length * 0.5f
				- engine.nozzleDepth
		};
	}
	return result;
}

} // namespace

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
	const auto& definition = config.spaceship().get(shipId);
	const float bodyScale = radius;

	ModelAndMounts result;
	result.modelId = modelManager.loadModel(definition.modelPath);
	result.bodyScale = bodyScale;
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
		shipId,
		params.radius
	);

	SpawnedSpaceship assembly;
	assembly.entity = context.registry.create();

	context.registry.emplace<Position>(assembly.entity, params.position);
	context.registry.emplace<Velocity>(assembly.entity);
	context.registry.emplace<Rotation>(assembly.entity, params.rotation);
	context.registry.emplace<CollisionBody>(assembly.entity, params.radius);
	context.registry.emplace<RenderBody>(assembly.entity, RenderBody{geometry.modelId, params.bodyColor, geometry.bodyScale});
	context.registry.emplace<faction::Faction>(assembly.entity, faction::Faction{params.faction});
	context.registry.emplace<tag::Targetable>(assembly.entity);
	context.registry.emplace<tag::Spaceship>(assembly.entity);
	context.registry.emplace<tag::Shaded>(assembly.entity);
	context.registry.emplace<tag::RotationSyncModel>(assembly.entity);
	context.registry.emplace<tag::effect::DropDebris>(assembly.entity);
	context.registry.emplace<SpawnsTrailParticles>(
		assembly.entity,
		makeShipTrailParticles(geometry.engines, geometry.bodyScale)
	);

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
