#include "systems.hpp"
#include "game_context.hpp"
#include "entities.hpp"
#include "components/effect.hpp"
#include "components/physics.hpp"
#include "components/render.hpp"
#include "components/lifetime.hpp"
#include "raymath.h"
#include <algorithm>

void systems::UpdateTrails::update(GameContext &context, float dt) {
	// 1. Single-emitter trails (Bullets & Missiles)
	for (auto [entity, trail] : context.registry.view<HasSimpleTrail>().each()) {
		// Age existing nodes
		std::uint8_t validCount = 0;
		for (std::size_t i = 0; i < trail.count; ++i) {
			trail.nodes[i].age += dt;
			if (trail.nodes[i].age < trail.maxAge) {
				trail.nodes[i].alpha = std::max(0.0f, 1.0f - (trail.nodes[i].age / trail.maxAge));
				trail.nodes[validCount++] = trail.nodes[i];
			}
		}
		trail.count = validCount;

		const auto *pos = context.registry.try_get<const Position>(entity);
		if (!pos) {
			continue;
		}

		if (trail.maxNodes <= 2) {
			// Bullet Tracer: 2-point camera billboard line
			trail.nodes[0] = {pos->value, 1.0f, 0.0f};
			trail.nodes[1] = {pos->prevValue, 0.2f, dt};
			trail.count = 2;
			continue;
		}

		// Missile Ribbon Trail
		const float distMoved = trail.hasLastRecordedPos
			? Vector3Distance(pos->value, trail.lastRecordedPos)
			: 999.0f;
		if (distMoved >= trail.minDistance) {
			const std::size_t toKeep = std::min(static_cast<std::size_t>(trail.count), static_cast<std::size_t>(trail.maxNodes - 1));
			for (std::size_t i = toKeep; i > 0; --i) {
				trail.nodes[i] = trail.nodes[i - 1];
			}
			trail.nodes[0] = {pos->value, 1.0f, 0.0f};
			trail.count = static_cast<std::uint8_t>(toKeep + 1);
			trail.lastRecordedPos = pos->value;
			trail.hasLastRecordedPos = true;
		}
	}

	// 2. Multi-emitter trails (Spaceship Thrusters)
	for (auto [entity, trail] : context.registry.view<HasMultiTrail>().each()) {
		const auto *pos = context.registry.try_get<const Position>(entity);
		const auto *rot = context.registry.try_get<const Rotation>(entity);
		const Quaternion orientation = rot ? rot->value : QuaternionIdentity();

		for (std::size_t e = 0; e < trail.emitterCount; ++e) {
			auto &emitter = trail.emitters[e];

			// Age existing nodes
			std::uint8_t validCount = 0;
			for (std::size_t i = 0; i < emitter.count; ++i) {
				emitter.nodes[i].age += dt;
				if (emitter.nodes[i].age < trail.maxAge) {
					emitter.nodes[i].alpha = std::max(0.0f, 1.0f - (emitter.nodes[i].age / trail.maxAge));
					emitter.nodes[validCount++] = emitter.nodes[i];
				}
			}
			emitter.count = validCount;

			if (!pos) {
				continue;
			}

			// Calculate exact nozzle world position
			const Vector3 nozzleWorld = pos->value + Vector3RotateByQuaternion(emitter.localOffset, orientation);

			const float distMoved = emitter.hasLastRecordedPos
				? Vector3Distance(nozzleWorld, emitter.lastRecordedPos)
				: 999.0f;
			if (distMoved >= trail.minDistance) {
				const std::size_t toKeep = std::min(static_cast<std::size_t>(emitter.count), static_cast<std::size_t>(trail.maxNodes - 1));
				for (std::size_t i = toKeep; i > 0; --i) {
					emitter.nodes[i] = emitter.nodes[i - 1];
				}
				emitter.nodes[0] = {nozzleWorld, 1.0f, 0.0f};
				emitter.count = static_cast<std::uint8_t>(toKeep + 1);
				emitter.lastRecordedPos = nozzleWorld;
				emitter.hasLastRecordedPos = true;
			}
		}
	}
}
