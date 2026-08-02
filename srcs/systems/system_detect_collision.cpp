#include "systems.hpp"
#include "utils.hpp"
#include "events.hpp"
#include "collision_algorithm.hpp"

#include <algorithm>
#include <vector>
#include <iostream>

namespace {
	// Helper function goes here
	// getCollisionRadius goes to CollisionBodyManager
	struct EntityData {
		entt::entity id;
		Vector3 pos;
		Vector3 vel;
		float rad;
		int faction;
		const CollisionBodyModel *collisionBodyModel;
		const RenderBody *renderBody;
	};

	struct MeshCollisionResult {
		bool usesMeshNarrowPhase;
		std::optional<CollisionHit> hit;
	};

	bool usesMeshCollision(const EntityData &entity)
	{
		return entity.collisionBodyModel != nullptr && entity.renderBody != nullptr;
	}

	MeshCollisionResult processMeshCollision(
		const GameContext &context,
		const EntityData &A,
		const EntityData &B,
		const CollisionInterval &interval
	)
	{
		const bool AUsesMesh = usesMeshCollision(A);
		const bool BUsesMesh = usesMeshCollision(B);
		if (AUsesMesh == BUsesMesh)
		{
			// Mesh versus mesh remains on the conservative sphere path for now.
			return MeshCollisionResult{false, std::nullopt};
		}

		const EntityData *meshEntity = AUsesMesh ? &A : &B;
		const EntityData *sphereEntity = AUsesMesh ? &B : &A;
		const CollisionModel &collisionModel = context.collisionBodyManager.getCollisionModel(
			meshEntity->collisionBodyModel->modelID
		);
		const CollisionMeshInstance meshInstance{
			meshEntity->pos,
			meshEntity->pos + meshEntity->vel,
			meshEntity->renderBody->translation,
			meshEntity->renderBody->scale,
			meshEntity->renderBody->rotation
		};
		const std::optional<CollisionHit> hit = sweepSphereAgainstMesh(
			collisionModel,
			meshInstance,
			sphereEntity->pos,
			sphereEntity->vel,
			sphereEntity->rad,
			interval
		);
		return MeshCollisionResult{true, hit};
	}
}

void ecs_systems::detectEntityCollision(GameContext& context, float dt) {
	std::vector<EntityData> entities;

	for (auto [entity, position, body] : context.registry.view<Position, CollisionBody>().each()) {
		Vector3 velocity = {0, 0, 0};
		PrevPosition *prev = context.registry.try_get<PrevPosition>(entity);
		if (prev != nullptr)
			velocity = position.value - prev->value;

		// only exclude bullet - bullet to prevent bullet collision
		// + allow friendly fire
		int faction = context.registry.any_of<tag::Bullet>(entity) << 0;
		const CollisionBodyModel *collisionBodyModel = context.registry.try_get<CollisionBodyModel>(entity);
		const RenderBody *renderBody = context.registry.try_get<RenderBody>(entity);
		float effectiveRadius = body.radius;
		if (collisionBodyModel != nullptr && renderBody != nullptr)
		{
			const float proxyRadius = context.collisionBodyManager.getCollisionRadius(
				collisionBodyModel->modelID,
				renderBody->translation,
				renderBody->scale,
				renderBody->rotation
			);
			effectiveRadius = std::max(effectiveRadius, proxyRadius);
		}

		entities.emplace_back(EntityData{
			entity,
			position.value - velocity,
			velocity,
			effectiveRadius,
			faction,
			collisionBodyModel,
			renderBody
		});
	}
	for (std::size_t i = 0; i < entities.size(); ++i) {
		const EntityData &A = entities[i];

		for (std::size_t j = i + 1; j < entities.size(); ++j) {
			const EntityData &B = entities[j];

			if ((A.faction & B.faction) != 0)
				continue;

			const float combinedRadius = A.rad + B.rad;
			std::optional<CollisionInterval> interval = calculateCollisionInterval(
				A.pos,
				A.vel,
				B.pos,
				B.vel,
				combinedRadius
			);
			if (!willCollide(interval, 1.0f))
				continue;

			// all the below code throw into the processMeshCollision, this code here should stay clean and next step is just itepereing and creating collision event
			const MeshCollisionResult meshCollision = processMeshCollision(context, A, B, *interval);
			if (meshCollision.usesMeshNarrowPhase && !meshCollision.hit)
				continue;

			const float collisionDt = meshCollision.hit
				? meshCollision.hit->collisionDt
				: std::max(interval->collisionStartDt, 0.0f);
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
