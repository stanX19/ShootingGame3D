#include "shoot_3d.hpp"

// deprecated
void spawBullet(entt::registry& registry, const Vector3& pos, const Vector3& dir, float damage, Color color) {
    entt::entity bullet = registry.create();
    registry.emplace<Position>(bullet, pos);
    registry.emplace<Velocity>(bullet, Vector3Scale(dir, 40.0f));
    registry.emplace<Body>(bullet, 0.2f, color);
    registry.emplace<Damage>(bullet, damage);
    registry.emplace<Lifetime>(bullet, 10.0f);
    registry.emplace<HP>(bullet, 1.0f, 1.0f);
    registry.emplace<Bullet>(bullet);
}