#include "shoot_3d.hpp"

void ecs_systems::playerShootControl(entt::registry &registry) {
	auto view = registry.view<Player, BulletWeapon>();

	for (auto entity : view)
	{
		BulletWeapon &bulletWeapon = view.get<BulletWeapon>(entity);

		bulletWeapon.firing = (IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_LEFT_BUTTON));
	}
}