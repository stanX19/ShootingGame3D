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

	void emplaceRandomWeapon(GameContext &context, entt::entity turret);
	void emplaceRandomWeapon(GameContext &context, entt::entity turret, int value);
}

namespace weapon::utils {
	void assureBulletTypes(entt::registry &registry);
	void hookWeaponDestructor(GameContext &context);
	void setUpRegistry(GameContext &context);
}

#endif // WEAPONS_HPP
