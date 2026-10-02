#ifndef WEAPONS_HPP
#define WEAPONS_HPP
#include "includes.hpp"
#include "components/weapon.hpp"
#include "components/render.hpp"
#include "components/combat.hpp"
#include "components/effect.hpp"
#include "components/identity.hpp"
#include "components/collision.hpp"
#include "components/lifetime.hpp"
#include "components/sound.hpp"
#include "components/spaceship.hpp"
#include "game_context.hpp"
#include <vector>
#include <string>
#include <cmath>
#include <iostream>

namespace weapon {
	void emplaceConfiguredWeapon(GameContext &context, entt::entity entity, const nlohmann::json &def);

	void emplaceGenericBullet(GameContext &context, entt::entity entity, const GameConfig& cfg);
	void emplaceGenericLazer(GameContext &context, entt::entity entity, const GameConfig& cfg);
	void emplaceGenericMissile(GameContext &context, entt::entity entity, const GameConfig& cfg);

	void emplaceWeaponMachineGun(GameContext &context, entt::entity entity, const GameConfig& cfg);
	void emplaceWeaponBasic(GameContext &context, entt::entity entity, const GameConfig& cfg);
	void emplaceWeaponSniper(GameContext &context, entt::entity entity, const GameConfig& cfg);
	void emplaceWeaponBurstSniper(GameContext &context, entt::entity entity, const GameConfig& cfg);
	void emplaceWeaponShotgun(GameContext &context, entt::entity entity, const GameConfig& cfg);
	void emplaceWeaponBigBall(GameContext &context, entt::entity entity, const GameConfig& cfg);
	
	void emplaceWeaponLazerBasic(GameContext &context, entt::entity entity, const GameConfig& cfg);
	void emplaceWeaponLazerMachineGun(GameContext &context, entt::entity entity, const GameConfig& cfg);
	void emplaceWeaponLazerDeletor(GameContext &context, entt::entity entity, const GameConfig& cfg);
	void emplaceWeaponLazerShotgun(GameContext &context, entt::entity entity, const GameConfig& cfg);
	
	void emplaceWeaponMissileBasic(GameContext &context, entt::entity entity, const GameConfig& cfg);
	void emplaceWeaponMissileSwarm(GameContext &context, entt::entity entity, const GameConfig& cfg);
	void emplaceWeaponMissileTorpedo(GameContext &context, entt::entity entity, const GameConfig& cfg);
	void emplaceWeaponMissileNuke(GameContext &context, entt::entity entity, const GameConfig& cfg);
	void emplaceWeaponMissileSniper(GameContext &context, entt::entity entity, const GameConfig& cfg);
	void emplaceWeaponMissileFlares(GameContext &context, entt::entity entity, const GameConfig& cfg);

}

namespace weapon::utils {
	void assureBulletTypes(entt::registry &registry);
	void hookWeaponDestructor(GameContext &context);
	void setUpRegistry(GameContext &context);
}

#endif // WEAPONS_HPP
