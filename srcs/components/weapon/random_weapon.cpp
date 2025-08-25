#include "weapons.hpp"
#include "utils.hpp"

void weapon::emplaceRandomWeapon(GameContext &context, entt::entity turret) {
	emplaceRandomWeapon(context, turret, GetRandomValue(0, 100000));
}

void weapon::emplaceRandomWeapon(GameContext &context, entt::entity turret, int value) {
	std::vector<std::function<void(GameContext &, entt::entity)>> list{
		weapon::emplaceWeaponBasic,
		weapon::emplaceWeaponSniper,
		weapon::emplaceWeaponBurstSniper,
		weapon::emplaceWeaponMachineGun,
		weapon::emplaceWeaponShotgun,
		weapon::emplaceWeaponBigBall,
		weapon::emplaceWeaponLazerBasic,
		weapon::emplaceWeaponLazerMachineGun,
		weapon::emplaceWeaponLazerDeletor,
		weapon::emplaceWeaponLazerShotgun,
	};
	list[value % list.size()](context, turret);
}