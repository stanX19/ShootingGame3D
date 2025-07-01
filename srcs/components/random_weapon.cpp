#include "entities.hpp"
#include "utils.hpp"

void emplaceRandomWeapon(GameContext &context, entt::entity turret) {
	emplaceRandomWeapon(context, turret, GetRandomValue(0, 100000));
}

void emplaceRandomWeapon(GameContext &context, entt::entity turret, int value) {
	std::vector<std::function<void(GameContext &, entt::entity)>> list{
		emplaceWeaponBasic,
		emplaceWeaponSniper,
		emplaceWeaponBurstSniper,
		emplaceWeaponMachineGun
	};
	list[value % list.size()](context, turret);
}