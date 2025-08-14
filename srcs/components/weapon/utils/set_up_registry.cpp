#include "weapons.hpp"
#include "entt_utils.hpp"
#include <iostream>

void weapon::utils::setUpRegistry(GameContext &context) {
	weapon::utils::assureBulletTypes(context.registry);
	weapon::utils::hookWeaponDestructor(context);
}

static void onBulletWeaponDestroyed(entt::registry &registry, entt::entity entity) {
	GameContext &context = registry.ctx().get<GameContext&>();

	Weapon &weapon = registry.get<Weapon>(entity);
	if (context.templateReg.valid(weapon.bulletTemplate)) {
		context.templateReg.destroy(weapon.bulletTemplate);
		// std::cout << "deleted " << static_cast<int>(weapon.bulletTemplate) << std::endl;
	}
}

void weapon::utils::hookWeaponDestructor(GameContext &context) {
	context.registry.ctx().emplace<GameContext&>(context);
	context.registry.on_destroy<Weapon>().connect<&onBulletWeaponDestroyed>();
}