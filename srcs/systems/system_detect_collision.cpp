#include "systems.hpp"
#include "game_context.hpp"
#include "events.hpp"
#include "collision_algorithm.hpp"
#include "components/physics.hpp"
#include "components/collision.hpp"
#include "components/anchor.hpp"
#include "components/render.hpp"
#include "components/weapon.hpp"
#include "utils/algorithm_utils.hpp"

#include <algorithm>
#include <vector>
#include <iostream>

namespace {
	struct EntityData {
		entt::entity id;
		Vector3 pos;
		Vector3 vel;
		float rad;
		float maxReach;
		int faction;
		entt::entity assemblyRoot;
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

	struct OBB {
		Vector3 center;
		Vector3 axes[3];
		Vector3 extents; // half-widths
	};

	OBB getEntityOBB(const GameContext &context, const EntityData &entity, float t = 0.0f)
	{
		const CollisionModel &model = context.collisionBodyManager.getCollisionModel(
			entity.collisionBodyModel->modelID
		);
		const Vector3 currentPos = entity.pos + entity.vel * t;
		const Quaternion rot = entity.renderBody->rotation;
		const Vector3 scale = entity.renderBody->scale;

		const Vector3 boxMin = model.bounds.min;
		const Vector3 boxMax = model.bounds.max;
		const Vector3 localCenter = (boxMin + boxMax) * 0.5f;
		const Vector3 extents = (boxMax - boxMin) * 0.5f * scale;

		const Vector3 translationOffset = entity.renderBody->translation;
		const Vector3 center = currentPos + Vector3RotateByQuaternion(translationOffset + localCenter * scale, rot);

		const Matrix rotMat = QuaternionToMatrix(rot);
		const Vector3 axes[3] = {
			Vector3{rotMat.m0, rotMat.m1, rotMat.m2},
			Vector3{rotMat.m4, rotMat.m5, rotMat.m6},
			Vector3{rotMat.m8, rotMat.m9, rotMat.m10}
		};

		return OBB{center, {axes[0], axes[1], axes[2]}, extents};
	}

	bool testOBBOverlap(const OBB &a, const OBB &b)
	{
		constexpr float epsilon = 1e-5f;
		Matrix R;
		Matrix absR;
		const Vector3 t = b.center - a.center;
		const Vector3 tA = {
			Vector3DotProduct(t, a.axes[0]),
			Vector3DotProduct(t, a.axes[1]),
			Vector3DotProduct(t, a.axes[2])
		};

		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				const float dot = Vector3DotProduct(a.axes[i], b.axes[j]);
				*(&R.m0 + i * 4 + j) = dot;
				*(&absR.m0 + i * 4 + j) = std::fabs(dot) + epsilon;
			}
		}

		// Test axes L = A0, A1, A2
		for (int i = 0; i < 3; ++i) {
			const float ra = (&a.extents.x)[i];
			const float rb = b.extents.x * (&absR.m0)[i * 4 + 0] +
			                 b.extents.y * (&absR.m0)[i * 4 + 1] +
			                 b.extents.z * (&absR.m0)[i * 4 + 2];
			if (std::fabs((&tA.x)[i]) > ra + rb) return false;
		}

		// Test axes L = B0, B1, B2
		for (int j = 0; j < 3; ++j) {
			const float ra = a.extents.x * (&absR.m0)[0 * 4 + j] +
			                 a.extents.y * (&absR.m0)[1 * 4 + j] +
			                 a.extents.z * (&absR.m0)[2 * 4 + j];
			const float rb = (&b.extents.x)[j];
			const float tB = tA.x * (&R.m0)[0 * 4 + j] +
			                 tA.y * (&R.m0)[1 * 4 + j] +
			                 tA.z * (&R.m0)[2 * 4 + j];
			if (std::fabs(tB) > ra + rb) return false;
		}

		// Test 9 cross products A_i x B_j
		if (std::fabs(tA.z * R.m4 - tA.y * R.m8) >
		    a.extents.y * absR.m8 + a.extents.z * absR.m4 +
		    b.extents.y * absR.m2 + b.extents.z * absR.m1) return false;

		if (std::fabs(tA.z * R.m5 - tA.y * R.m9) >
		    a.extents.y * absR.m9 + a.extents.z * absR.m5 +
		    b.extents.x * absR.m2 + b.extents.z * absR.m0) return false;

		if (std::fabs(tA.z * R.m6 - tA.y * R.m10) >
		    a.extents.y * absR.m10 + a.extents.z * absR.m6 +
		    b.extents.x * absR.m1 + b.extents.y * absR.m0) return false;

		if (std::fabs(tA.x * R.m8 - tA.z * R.m0) >
		    a.extents.x * absR.m8 + a.extents.z * absR.m0 +
		    b.extents.y * absR.m6 + b.extents.z * absR.m5) return false;

		if (std::fabs(tA.x * R.m9 - tA.z * R.m1) >
		    a.extents.x * absR.m9 + a.extents.z * absR.m1 +
		    b.extents.x * absR.m6 + b.extents.z * absR.m4) return false;

		if (std::fabs(tA.x * R.m10 - tA.z * R.m2) >
		    a.extents.x * absR.m10 + a.extents.z * absR.m2 +
		    b.extents.x * absR.m5 + b.extents.y * absR.m4) return false;

		if (std::fabs(tA.y * R.m0 - tA.x * R.m4) >
		    a.extents.x * absR.m4 + a.extents.y * absR.m0 +
		    b.extents.y * absR.m10 + b.extents.z * absR.m9) return false;

		if (std::fabs(tA.y * R.m1 - tA.x * R.m5) >
		    a.extents.x * absR.m5 + a.extents.y * absR.m1 +
		    b.extents.x * absR.m10 + b.extents.z * absR.m8) return false;

		if (std::fabs(tA.y * R.m2 - tA.x * R.m6) >
		    a.extents.x * absR.m6 + a.extents.y * absR.m2 +
		    b.extents.x * absR.m9 + b.extents.y * absR.m8) return false;

		return true;
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
		if (!AUsesMesh && !BUsesMesh)
		{
			return MeshCollisionResult{false, std::nullopt};
		}

		const EntityData *meshEntity = nullptr;
		const EntityData *sphereEntity = nullptr;
		if (AUsesMesh && BUsesMesh)
		{
			const float checkTime = std::max(interval.collisionStartDt, 0.0f);
			const OBB obbA = getEntityOBB(context, A, checkTime);
			const OBB obbB = getEntityOBB(context, B, checkTime);
			if (!testOBBOverlap(obbA, obbB))
			{
				return MeshCollisionResult{true, std::nullopt};
			}

			meshEntity = (A.rad >= B.rad) ? &A : &B;
			sphereEntity = (A.rad >= B.rad) ? &B : &A;
		}
		else
		{
			meshEntity = AUsesMesh ? &A : &B;
			sphereEntity = AUsesMesh ? &B : &A;
		}
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

void systems::DetectEntityCollision::update(GameContext& context, float dt) {
	std::vector<EntityData> targets;
	std::vector<EntityData> projectiles;
	targets.reserve(128);
	projectiles.reserve(1024);

	for (auto [entity, position, body] : context.registry.view<Position, CollisionBody>().each()) {
		const auto [collisionBodyModel, renderBody, assembly] =
			context.registry.try_get<CollisionBodyModel, RenderBody, collision::Assembly>(entity);

		const Vector3 velocity = position.value - position.prevValue;
		const bool isBullet = context.registry.any_of<tag::Bullet>(entity);
		const int faction = isBullet ? 1 : 0;
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

		const entt::entity assemblyRoot = assembly ? assembly->root : entt::null;

		const float velLen = Vector3Length(velocity);
		EntityData ed{
			entity,
			position.prevValue,
			velocity,
			effectiveRadius,
			effectiveRadius + velLen,
			faction,
			assemblyRoot,
			collisionBodyModel,
			renderBody
		};

		if (isBullet)
			projectiles.emplace_back(std::move(ed));
		else
			targets.emplace_back(std::move(ed));
	}

	auto testPair = [&](const EntityData &A, const EntityData &B) {
		if (A.assemblyRoot != entt::null && A.assemblyRoot == B.assemblyRoot)
			return;

		const float dx = A.pos.x - B.pos.x;
		const float dy = A.pos.y - B.pos.y;
		const float dz = A.pos.z - B.pos.z;
		const float maxDist = A.maxReach + B.maxReach;
		if (dx * dx + dy * dy + dz * dz > maxDist * maxDist)
			return;

		const float combinedRadius = A.rad + B.rad;
		std::optional<CollisionInterval> interval = calculateCollisionInterval(
			A.pos,
			A.vel,
			B.pos,
			B.vel,
			combinedRadius
		);
		if (!willCollide(interval, 1.0f))
			return;

		const MeshCollisionResult meshCollision = processMeshCollision(context, A, B, *interval);
		if (meshCollision.usesMeshNarrowPhase && !meshCollision.hit)
			return;

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
	};

	// 1. Targets vs Targets
	for (std::size_t i = 0; i < targets.size(); ++i) {
		for (std::size_t j = i + 1; j < targets.size(); ++j) {
			testPair(targets[i], targets[j]);
		}
	}

	// 2. Projectiles vs Targets (zero bullet-bullet checks!)
	for (const auto &proj : projectiles) {
		for (const auto &tgt : targets) {
			testPair(proj, tgt);
		}
	}
}

