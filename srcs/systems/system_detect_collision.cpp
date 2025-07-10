#include "systems.hpp"
#include "utils.hpp"
#include "events.hpp"
#include <vector>
#include <iostream>

void ecs_systems::detectEntityCollision(GameContext& context, float dt) {
	struct EntityData {
		entt::entity id;
		Vector3 position;
		Vector3 velocity;
		float radius;
		int faction;
	};

	std::vector<EntityData> entities;

	for (auto [entity, position, body] : context.registry.view<Position, CollisionBody>().each()) {
		Vector3 velocity = {0, 0, 0};
		if (auto* prev = context.registry.try_get<PrevPosition>(entity))
			velocity = position.value - prev->value;

		// only exclude bullet - bullet to prevent bullet collision
		// + allow friendly fire
		int faction = context.registry.any_of<tag::Bullet>(entity) << 0;

		entities.emplace_back(EntityData{
			entity,
			position.value - velocity,
			velocity,
			body.radius,
			faction
		});
	}

	for (size_t i = 0; i < entities.size(); ++i) {
		const auto& A = entities[i];

		for (size_t j = i + 1; j < entities.size(); ++j) {
			const auto& B = entities[j];

			if ((A.faction & B.faction) != 0)
				continue;

			float combinedRadius = A.radius + B.radius;
			if (willCollide(A.position, A.velocity, B.position, B.velocity, combinedRadius, 1.0f)) {
				context.dispatcher.enqueue<event::CollisionEvent>({&context, A.id, B.id, dt});
			}
		}
	}
}
