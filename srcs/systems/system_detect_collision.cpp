#include "systems.hpp"
#include "utils.hpp"
#include "events.hpp"

#include <algorithm>
#include <vector>
#include <iostream>

void ecs_systems::detectEntityCollision(GameContext& context, float dt) {
	struct EntityData {
		entt::entity id;
		Vector3 pos;
		Vector3 vel;
		float rad;
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

			float combinedRadius = A.rad + B.rad;
			std::optional<CollisionInterval> interval = calculateCollisionInterval(
				A.pos,
				A.vel,
				B.pos,
				B.vel,
				combinedRadius
			);
			if (!willCollide(interval, 1.0f))
				continue;

			float collisionDt = std::max(interval->collisionStartDt, 0.0f);
			context.dispatcher.enqueue<event::CollisionEvent>(event::CollisionEvent{
				&context,
				event::CollisionParty{A.id, A.pos + A.vel * collisionDt, A.vel / dt},
				event::CollisionParty{B.id, B.pos + B.vel * collisionDt, B.vel / dt},
				dt,
				collisionDt}
			);
		}
	}
}
