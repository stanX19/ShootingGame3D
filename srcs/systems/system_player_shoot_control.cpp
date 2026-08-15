#include "systems.hpp"

namespace {
	void setFireRequestStatus(entt::registry &registry, entt::entity entity, bool isFiring) {
		if (isFiring)
			registry.emplace_or_replace<tag::weapon::FireRequest>(entity);
	}

	static constexpr std::array<KeyboardKey, 10> numberKeys = {
        KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR,
        KEY_FIVE, KEY_SIX, KEY_SEVEN, KEY_EIGHT,
		KEY_NINE, KEY_ZERO
    };
}

void ecs_systems::playerShootControl(GameContext &context, [[maybe_unused]] float dt) {
	auto playerView = context.registry.view<tag::weapon::PlayerControlledFire>();
	auto weaponView = context.registry.view<WeaponParent, tag::weapon::IsWeapon>();

	for (auto player : playerView)
	{
		std::vector<entt::entity> weaponEntities;

		bool globalFire = IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_LEFT_BUTTON);

		if (context.registry.valid(player) && context.registry.all_of<tag::weapon::IsWeapon>(player)) {
			weaponEntities.push_back(player);
		}
		for (auto [weaponEntity, weaponParent] : weaponView.each())
		{
			if (weaponParent.parent == player)
				weaponEntities.push_back(weaponEntity);
		}
		for (size_t i = 0; i < weaponEntities.size(); ++i) {
			bool shouldFire = globalFire;
			bool numberKeyPressed = (i < numberKeys.size() && IsKeyDown(numberKeys[i]));

			if (context.registry.all_of<tag::weapon::IsSpecialWeapon>(weaponEntities[i]))
				shouldFire = IsKeyDown(KEY_E);
			setFireRequestStatus(context.registry, weaponEntities[i], shouldFire || numberKeyPressed);
		}
	}
}
