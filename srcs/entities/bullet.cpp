#include "shoot_3d.hpp"

static const Vector3 arenaVec = {ARENA_SIZE, ARENA_SIZE, ARENA_SIZE};

void spawnBullet(entt::registry &registry, Position pos, Velocity velocity, HP hp,
				 Damage damage, float rad, Color color, Lifetime lifetime)
{
	entt::entity bullet = registry.create();
	registry.emplace<Position>(bullet, pos);
	registry.emplace<Velocity>(bullet, velocity);
	registry.emplace<CollisionBody>(bullet, rad);
	registry.emplace<RenderBody>(bullet, rad, color);
	registry.emplace<Damage>(bullet, damage);
	registry.emplace<Lifetime>(bullet, lifetime);
	registry.emplace<DisappearBound>(bullet, arenaVec * -2, arenaVec * 2);
	registry.emplace<HP>(bullet, hp);
	registry.emplace<tag::Bullet>(bullet);
}