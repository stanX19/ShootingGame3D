#ifndef WEAPONS_HPP
#define WEAPONS_HPP
#include "includes.hpp"
#include "components.hpp"
#include "game_context.hpp"
#include <vector>
#include <string>
#include <cmath>
#include <iostream>

namespace weapon {
	void emplaceWeaponMachineGun(GameContext &context, entt::entity entity);
	void emplaceWeaponBasic(GameContext &context, entt::entity entity);
	void emplaceWeaponSniper(GameContext &context, entt::entity entity);
	void emplaceWeaponBurstSniper(GameContext &context, entt::entity entity);
	void emplaceWeaponShotgun(GameContext &context, entt::entity entity);
	void emplaceWeaponBigBall(GameContext &context, entt::entity entity);
	
	void emplaceWeaponLazerBasic(GameContext &context, entt::entity entity);
	void emplaceWeaponLazerMachineGun(GameContext &context, entt::entity entity);
	void emplaceWeaponLazerDeletor(GameContext &context, entt::entity entity);
	void emplaceWeaponLazerShotgun(GameContext &context, entt::entity entity);
	
	void emplaceWeaponMissileBasic(GameContext &context, entt::entity entity);
	void emplaceWeaponMissileSwarm(GameContext &context, entt::entity entity);
	void emplaceWeaponMissileTorpedo(GameContext &context, entt::entity entity);
	void emplaceWeaponMissileNuke(GameContext &context, entt::entity entity);
	void emplaceWeaponMissileSniper(GameContext &context, entt::entity entity);
	void emplaceWeaponMissileFlares(GameContext &context, entt::entity entity);

}

namespace weapon::utils {
	void assureBulletTypes(entt::registry &registry);
	void hookWeaponDestructor(GameContext &context);
	void setUpRegistry(GameContext &context);
}

#endif // WEAPONS_HPP
