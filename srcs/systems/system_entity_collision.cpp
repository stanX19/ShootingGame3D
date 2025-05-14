#include "shoot_3d.hpp"
#include <unordered_map>
#include <vector>
#include <tuple>
#include <cmath>

static bool hadCollision(const Vector3 &posA, const Vector3 &velA, const Vector3 &posB, const Vector3 &velB, float collisionDistance)
{
	Vector3 relPos = (posA - velA) - (posB - velB);
	Vector3 relVel = velA - velB;

	float a = Vector3DotProduct(relVel, relVel);
	float b = 2.0f * Vector3DotProduct(relPos, relVel);
	float c = Vector3DotProduct(relPos, relPos) - collisionDistance * collisionDistance;

	float discriminant = b * b - 4 * a * c;

	if (discriminant < 0.0f || a == 0.0f) // if no solution or no relative movement
		return (c == 0.0);				  // return (is currently overlaping)

	float sqrtD = sqrtf(discriminant);
	float t1 = (-b - sqrtD) / (2.0f * a);
	float t2 = (-b + sqrtD) / (2.0f * a);

	// Collision happens during the frame
	return ((t1 >= 0.0f && t1 <= 1.0f) || (t2 >= 0.0f && t2 <= 1.0f));
}

void ecs_systems::entityCollision(entt::registry &registry, float dt)
{
	// Step 1: Collect attacking and target entities
	struct EntityData
	{
		entt::entity id;
		Vector3 position;
		Vector3 velocity;
		float radius;
		float damage;
	};

	std::vector<EntityData> entities;

	auto view = registry.view<Position, CollisionBody>();
	for (auto entity : view)
	{
		entities.emplace_back(EntityData{
			entity,
			view.get<Position>(entity).value,
			registry.all_of<Velocity>(entity)? registry.get<Velocity>(entity).value * dt: Vector3{0, 0, 0},
			view.get<CollisionBody>(entity).radius,
			registry.all_of<Damage>(entity)? registry.get<Damage>(entity).value * dt: 0,
		});
	}

	// Step 2: Detect and queue collisions
	std::map<entt::entity, float> damageMap; // (target, damage)

	for (size_t i = 0; i < entities.size(); ++i)
	{
		const auto &A = entities[i];

		for (size_t j = i + 1; j < entities.size(); ++j)
		{
			const auto &B = entities[j];
			
			float combinedRadius = A.radius + B.radius;
			if (hadCollision(A.position, A.velocity, B.position, B.velocity, combinedRadius))
			{
				damageMap[B.id] += A.damage;
				damageMap[A.id] += B.damage;
			}
		}
	}

	// Step 3: Apply damage in bulk (deferred)
	for (const auto &[target, damage] : damageMap)
	{
		if (registry.all_of<HP>(target))
		{
			auto &hp = registry.get<HP>(target);
			hp.value -= damage;
		}
	}
}