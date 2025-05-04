#include "shoot_3d.hpp"

void ecs_system::hp_regen(entt::registry& registry, float dt) {
    auto view = registry.view<HP, HPRegen>();

    for (auto entity : view) {
        HP& hp = view.get<HP>(entity);
		HPRegen& regen = view.get<HPRegen>(entity);
		
		hp.value = Clamp(hp.value + regen.value * dt, 0, hp.maxValue);
    }
}
