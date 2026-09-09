#include "systems.hpp"
#include "game_context.hpp"
#include "components/physics.hpp"
#include "components/render.hpp"
#include "components/weapon.hpp"
#include "utils/vector_rotation_utils.hpp"

namespace {
	void rotationSyncModel(GameContext &context, [[maybe_unused]] float dt) {
		auto view = context.registry.view<const Rotation, RenderBody, tag::RotationSyncModel>();
		for (auto entity : view) {
			const Rotation& rotation = view.get<const Rotation>(entity);
			RenderBody& body = view.get<RenderBody>(entity);
			body.rotation = rotation.value;
		}
	}

	void aimDirectionSyncModel(GameContext &context, [[maybe_unused]] float dt) {
		auto viewAimOnly = context.registry.view<const AimDirection, RenderBody, tag::AimDirectionSyncModel>(entt::exclude<Rotation>);
		for (auto entity : viewAimOnly) {
			const AimDirection& aim = viewAimOnly.get<const AimDirection>(entity);
			RenderBody& body = viewAimOnly.get<RenderBody>(entity);
			body.rotation = vector3ToRotation(aim.value);
		}

		auto viewAimAndRot = context.registry.view<const AimDirection, const Rotation, RenderBody, tag::AimDirectionSyncModel>();
		for (auto entity : viewAimAndRot) {
			const AimDirection& aim = viewAimAndRot.get<const AimDirection>(entity);
			const Rotation& rotation = viewAimAndRot.get<const Rotation>(entity);
			RenderBody& body = viewAimAndRot.get<RenderBody>(entity);
			body.rotation = vector3ToRotation(aim.value, rotation.value);
		}
	}

	void velocitySyncModelRot(GameContext &context, [[maybe_unused]] float dt) {
		for (auto [entity, vel, body] : context.registry.view<const Velocity, RenderBody, tag::VelocitySyncModelRot>().each()) {
			body.rotation = vector3ToRotation(vel.value);
		}
	}

	void velocitySyncRot(GameContext &context, [[maybe_unused]] float dt) {
		for (auto [entity, vel, rot] : context.registry.view<const Velocity, Rotation, tag::VelocitySyncRot>().each()) {
			rot.value = vector3ToRotation(vel.value);
		}
	}
}

void systems::SyncModelRotation::update(GameContext &context, [[maybe_unused]] float dt) {
	rotationSyncModel(context, dt);
	aimDirectionSyncModel(context, dt);
	velocitySyncModelRot(context, dt);
	velocitySyncRot(context, dt);
}

