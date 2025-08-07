#include "entities.hpp"
#include "constants.hpp"

static const Vector3 arenaVec = {ARENA_SIZE, ARENA_SIZE, ARENA_SIZE};

void spawnBullet(GameContext &context, Position pos, Velocity velocity, HP hp,
				 Damage damage, float rad, Color color, Lifespan lifetime, ScoreParent scoreParent)
{
	t_model_id sphereModel = context.meshManager.createSphere();
	entt::entity bullet = context.registry.create();
	context.registry.emplace<Position>(bullet, pos);
	context.registry.emplace<Velocity>(bullet, velocity);
	context.registry.emplace<CollisionBody>(bullet, rad);
	context.registry.emplace<RenderBody>(bullet, RenderBody{sphereModel, color, rad});
	context.registry.emplace<ModelStrech>(bullet, 1.0f);
	context.registry.emplace<Damage>(bullet, damage);
	context.registry.emplace<Lifespan>(bullet, lifetime);
	context.registry.emplace<DisappearBound>(bullet, arenaVec * -2, arenaVec * 2);
	context.registry.emplace<HP>(bullet, hp);
	context.registry.emplace<ScoreParent>(bullet, scoreParent);
	context.registry.emplace<tag::Bullet>(bullet);
	context.registry.emplace<tag::VelocitySyncModelRot>(bullet);
}