#include "shoot_3d.hpp"

void ecs_system::ammo_reload(entt::registry& registry, float dt) {
    auto view = registry.view<Ammo, AmmoReload>();

    for (auto entity : view) {
        Ammo& ammo = view.get<Ammo>(entity);
		AmmoReload& reload = view.get<AmmoReload>(entity);
		
		ammo.value = Clamp(ammo.value + reload.value * dt, 0, ammo.maxValue);
    }
}
