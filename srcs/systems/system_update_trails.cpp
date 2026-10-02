#include "systems.hpp"
#include "game_context.hpp"
#include "components/effect.hpp"
#include "components/physics.hpp"
#include "raymath.h"
#include <algorithm>

namespace {

void ageNodes(trail::Node *nodes, std::uint8_t &count, float maxAge, float dt) {
	std::uint8_t validCount = 0;
	for (std::size_t i = 0; i < count; ++i) {
		nodes[i].age += dt;
		if (nodes[i].age >= maxAge) {
			continue;
		}
		nodes[i].alpha = std::max(0.0f, 1.0f - (nodes[i].age / maxAge));
		nodes[validCount++] = nodes[i];
	}
	count = validCount;
}

void shiftNodesAndInsert(trail::Node *nodes, std::uint8_t &count, std::uint8_t maxNodes, const Vector3 &pos) {
	const std::size_t toKeep = std::min(static_cast<std::size_t>(count), static_cast<std::size_t>(maxNodes - 1));
	for (std::size_t i = toKeep; i > 0; --i) {
		nodes[i] = nodes[i - 1];
	}
	nodes[0] = {pos, 1.0f, 0.0f};
	count = static_cast<std::uint8_t>(toKeep + 1);
}

void updateTrack(
	trail::Node *nodes,
	std::uint8_t &count,
	std::uint8_t maxNodes,
	Vector3 &lastPos,
	bool &hasLastPos,
	const Vector3 &livePos,
	float minDistance,
	float maxAge,
	float dt
) {
	ageNodes(nodes, count, maxAge, dt);

	if (count == 0) {
		nodes[0] = {livePos, 1.0f, 0.0f};
		count = 1;
		lastPos = livePos;
		hasLastPos = true;
	}

	const float distMoved = hasLastPos
		? Vector3Distance(livePos, lastPos)
		: 999.0f;

	if (distMoved >= minDistance) {
		shiftNodesAndInsert(nodes, count, maxNodes, livePos);
		lastPos = livePos;
	}

	// Continuously weld node 0 to live position to eliminate 1-frame desync/gap
	nodes[0] = {livePos, 1.0f, 0.0f};
}

void updateSimpleTrail(effect::HasSimpleTrail &trail, const physics::Position *pos, float dt) {
	if (pos == nullptr) {
		ageNodes(trail.nodes, trail.count, trail.maxAge, dt);
		return;
	}

	if (trail.maxNodes <= 2) {
		trail.nodes[0] = {pos->value, 1.0f, 0.0f};
		trail.nodes[1] = {pos->prevValue, 0.2f, dt};
		trail.count = 2;
		return;
	}

	updateTrack(
		trail.nodes,
		trail.count,
		trail.maxNodes,
		trail.lastRecordedPos,
		trail.hasLastRecordedPos,
		pos->value,
		trail.minDistance,
		trail.maxAge,
		dt
	);
}

void updateMultiTrail(
	effect::HasMultiTrail &trail,
	const physics::Position *pos,
	const physics::Rotation *rot,
	float dt
) {
	if (pos == nullptr) {
		for (std::size_t e = 0; e < trail.emitterCount; ++e) {
			ageNodes(trail.emitters[e].nodes, trail.emitters[e].count, trail.maxAge, dt);
		}
		return;
	}

	const Quaternion orientation = (rot != nullptr) ? rot->value : QuaternionIdentity();

	for (std::size_t e = 0; e < trail.emitterCount; ++e) {
		auto &emitter = trail.emitters[e];
		const Vector3 nozzleWorld = pos->value + Vector3RotateByQuaternion(emitter.localOffset, orientation);
		updateTrack(
			emitter.nodes,
			emitter.count,
			trail.maxNodes,
			emitter.lastRecordedPos,
			emitter.hasLastRecordedPos,
			nozzleWorld,
			trail.minDistance,
			trail.maxAge,
			dt
		);
	}
}

} // namespace

void systems::UpdateTrails::update(GameContext &context, float dt) {
	for (auto [entity, trail] : context.registry.view<effect::HasSimpleTrail>().each()) {
		const auto *pos = context.registry.try_get<const physics::Position>(entity);
		updateSimpleTrail(trail, pos, dt);
	}

	for (auto [entity, trail] : context.registry.view<effect::HasMultiTrail>().each()) {
		const auto [pos, rot] = context.registry.try_get<const physics::Position, const physics::Rotation>(entity);
		updateMultiTrail(trail, pos, rot, dt);
	}
}

