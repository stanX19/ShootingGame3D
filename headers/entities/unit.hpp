#ifndef ENTITIES_UNIT_HPP
#define ENTITIES_UNIT_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "entities/turret.hpp"
#include "game_context.hpp"

namespace unit {

struct Loadout {
	std::vector<std::string> turretWeapons;
	std::string specialWeapon;
};

struct SpawnParams {
	Vector3 position{};
	faction::FacVal faction = faction::FAC_NONE;
	Color bodyColor = WHITE;
	Color turretColor = WHITE;
	Quaternion rotation = QuaternionIdentity();
	turret::TurretControlMode turretControl =
		turret::TurretControlMode::FollowParent;
	Loadout loadout;
};

struct SpawnedUnit {
	entt::entity entity = entt::null;
	std::vector<entt::entity> turrets;

	entt::entity turret(std::size_t index) const;
};

SpawnedUnit spawnConfiguredUnit(
	GameContext& context,
	std::string_view unitId,
	const SpawnParams& params
);

} // namespace unit

#endif // ENTITIES_UNIT_HPP
