#include "shoot_3d.hpp"

void add_bullet_weapon_with_ammo(entt::registry& registry, entt::entity entity) {
    BulletWeapon weapon;
    weapon.firing = false;
    weapon.shootCooldown = 0.2f;
    weapon.timeSinceLastShot = 0.0f;

    weapon.bulletData.hp = 1.0f;
    weapon.bulletData.dmg = 10.0f;
    weapon.bulletData.speed = 40.0f;
    weapon.bulletData.rad = 0.2f;
    weapon.bulletData.color = (registry.any_of<Body>(entity))? registry.get<Body>(entity).color: WHITE;
    weapon.bulletData.lifetime = 10.0f;

    Ammo ammo;
    ammo.value = 30.0f;
    ammo.maxValue = 30;

    AmmoReload reload;
    reload.value = 5.0f; // ammo/sec

    registry.emplace_or_replace<BulletWeapon>(entity, weapon);
    registry.emplace_or_replace<Ammo>(entity, ammo);
    registry.emplace_or_replace<AmmoReload>(entity, reload);
}
