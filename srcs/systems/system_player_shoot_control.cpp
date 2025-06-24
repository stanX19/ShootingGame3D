#include "shoot_3d.hpp"

void ecs_systems::playerShootControl(GameContext &context) {
	auto view = context.registry.view<tag::Player, BulletWeapon>();

	for (auto entity : view)
	{
		BulletWeapon &bulletWeapon = view.get<BulletWeapon>(entity);

		bulletWeapon.firing = (IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_LEFT_BUTTON));
	}
}