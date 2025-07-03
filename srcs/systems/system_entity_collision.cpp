#include "systems.hpp"
#include "utils.hpp"
#include <unordered_map>
#include <vector>
#include <tuple>
#include <cmath>
#include <iostream>

void ecs_systems::entityCollision(GameContext &context, float dt)
{
	// Step 1: Collect attacking and target entities
	struct EntityData
	{
		entt::entity id;
		Vector3 position;
		Vector3 velocity;
		float radius;
		float damage;
		int faction;
	};

	std::vector<EntityData> entities;

	for (auto [entity, position, body] : context.registry.view<Position, CollisionBody>().each())
	{
		Vector3 velocity = {0, 0, 0};
		PrevPosition *prevPosPtr = context.registry.try_get<PrevPosition>(entity);
		if (prevPosPtr)
			velocity = position.value - prevPosPtr->value;

		float damage = 0;
		Damage *dmgPtr = context.registry.try_get<Damage>(entity);
		if (dmgPtr)
			damage = dmgPtr->value * dt;

		entities.emplace_back(EntityData{
			entity,
			position.value - velocity,
			velocity,
			body.radius,
			damage,
			(context.registry.any_of<tag::Bullet>(entity) << 0),
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
			
			if ((A.faction & B.faction) != 0)  // allied faction
				continue;

			float combinedRadius = A.radius + B.radius;
			if (willCollide(A.position, A.velocity, B.position, B.velocity, combinedRadius, 1.0))
			{
				damageMap[B.id] += A.damage;
				damageMap[A.id] += B.damage;
				// std::cout << "Collided: " << static_cast<int>(A.id) << ' ' << (int)B.id << std::endl; 
			}
		}
	}

	// Step 3: Apply damage in bulk (deferred)
	for (const auto &[target, damage] : damageMap)
	{
		HP *hpPtr = context.registry.try_get<HP>(target);
		if (hpPtr)
		{
			hpPtr->value -= damage;
			// std::cout << "HP: " << hpPtr->value << "; damaged " << damage << std::endl;
		}
	}
}