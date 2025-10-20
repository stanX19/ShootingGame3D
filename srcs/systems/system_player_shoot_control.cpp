#include "systems.hpp"

namespace {
	void setFiringStatus(entt::registry &registry, entt::entity entity, bool isFiring) {
		if (isFiring) {
			registry.emplace_or_replace<tag::weapon::IsFiring>(entity);
		} else {
			registry.remove<tag::weapon::IsFiring>(entity);
		}
	}

	static constexpr std::array<KeyboardKey, 8> numberKeys = {
        KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR,
        KEY_FIVE, KEY_SIX, KEY_SEVEN, KEY_EIGHT
    };
}

void ecs_systems::playerShootControl(GameContext &context) {
	auto playerView = context.registry.view<tag::weapon::PlayerControlledFire>();
	auto weaponView = context.registry.view<WeaponParent, tag::weapon::IsWeapon>();

	for (auto player : playerView)
	{
		bool globalFire = IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_LEFT_BUTTON);
		std::vector<entt::entity> weaponEntities;

		if (context.registry.valid(player) && context.registry.all_of<tag::weapon::IsWeapon>(player)) {
			weaponEntities.push_back(player);
		}
		for (auto [weaponEntity, weaponParent] : weaponView.each())
		{
			if (weaponParent.parent == player)
				weaponEntities.push_back(weaponEntity);
		}
		for (size_t i = 0; i < weaponEntities.size() && i < numberKeys.size(); ++i) {
			setFiringStatus(context.registry, weaponEntities[i], globalFire || IsKeyDown(numberKeys[i]));
		}
	}
}
