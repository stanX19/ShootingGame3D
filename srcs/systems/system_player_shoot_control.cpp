#include "systems.hpp"

void ecs_systems::playerShootControl(GameContext &context) {
	auto view = context.registry.view<tag::Player>();

	for (auto entity : view)
	{
		if (IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
			context.registry.emplace_or_replace<tag::weapon::IsFiring>(entity);
		} else {
			context.registry.remove<tag::weapon::IsFiring>(entity);
		}
	}
}