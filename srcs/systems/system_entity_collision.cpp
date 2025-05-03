#include "shoot_3d.hpp"

void ecs_system::entity_collision(entt::registry& registry) {
    auto damageViews = registry.view<Position, Body, Damage>();
    auto entityViews = registry.view<Position, Body, HP>();

    for (auto attacker : damageViews) {
        const Position& posA = damageViews.get<Position>(attacker);
        const Body& bodyA = damageViews.get<Body>(attacker);
        const Damage& damage = damageViews.get<Damage>(attacker);
        int damageValue = damage.value;

        for (auto target : entityViews) {
            if (attacker == target) continue;

            const Position& posB = entityViews.get<Position>(target);
            const Body& bodyB = entityViews.get<Body>(target);

            float dx = posA.value.x - posB.value.x;
            float dy = posA.value.y - posB.value.y;
            float dz = posA.value.z - posB.value.z;
            float distSq = dx * dx + dy * dy + dz * dz;
            float radiusSum = bodyA.radius + bodyB.radius;

            if (distSq <= radiusSum * radiusSum) {
                HP& hp = registry.get<HP>(target);
                hp.value -= damageValue;
                break;  // stop after first hit
            }
        }
    }
}