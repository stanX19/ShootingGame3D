#ifndef SPACESHIP_FACTORY_HPP
#define SPACESHIP_FACTORY_HPP

#include <cstddef>
#include <string_view>
#include <vector>

#include "config/spaceship_config.hpp"
#include "entities/turret.hpp"
#include "game_context.hpp"

namespace spaceship::factory {

using EnginePoint = config::SpaceshipConfig::Engine;
using MountPoint = config::SpaceshipConfig::Mount;

struct ModelAndMounts {
	t_model_id modelId = 0;
	float bodyScale = 1.0f;
	float modelRadius = 0.0f;
	std::vector<EnginePoint> engines;
	std::vector<MountPoint> mounts;
};

struct SpawnParams {
	Vector3 position{};
	float radius = 1.0f;
	Color bodyColor = WHITE;
	Color turretColor = WHITE;
	Quaternion rotation = QuaternionIdentity();
	faction::FacVal faction = faction::FAC_NONE;
	turret::TurretControlMode turretControl = turret::TurretControlMode::FollowParent;
};

struct SpawnedSpaceship {
	entt::entity entity = entt::null;
	std::vector<entt::entity> turrets;

	entt::entity turret(std::size_t index) const;
};

ModelAndMounts getModelAndMounts(
	const GameConfig& config,
	ModelManager& modelManager,
	std::string_view shipId,
	float radius
);

SpawnedSpaceship spawnConfiguredSpaceship(
	GameContext& context,
	std::string_view shipId,
	const SpawnParams& params
);

} // namespace spaceship::factory

#endif // SPACESHIP_FACTORY_HPP
