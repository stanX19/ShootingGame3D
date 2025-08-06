#include "systems.hpp"
#include "utils.hpp"

static void rotationSyncModel(GameContext &context) {
	auto view = context.registry.view<Rotation, RenderBody, tag::RotationSyncModel>();
	for (auto entity : view) {
		Rotation& rotation = view.get<Rotation>(entity);
		RenderBody& body = view.get<RenderBody>(entity);
		body.rotation = rotation.value;
	}
}

static void aimDirectionSyncModel(GameContext &context) {
	auto viewAimOnly = context.registry.view<AimDirection, RenderBody, tag::AimDirectionSyncModel>(entt::exclude<Rotation>);
	for (auto entity : viewAimOnly) {
		AimDirection& aim = viewAimOnly.get<AimDirection>(entity);
		RenderBody& body = viewAimOnly.get<RenderBody>(entity);
		body.rotation = vector3ToRotation(aim.value);
	}

	auto viewAimAndRot = context.registry.view<AimDirection, Rotation, RenderBody, tag::AimDirectionSyncModel>();
	for (auto entity : viewAimAndRot) {
		AimDirection& aim = viewAimAndRot.get<AimDirection>(entity);
		Rotation& rotation = viewAimAndRot.get<Rotation>(entity);
		RenderBody& body = viewAimAndRot.get<RenderBody>(entity);
		body.rotation = vector3ToRotation(aim.value, rotation.value);
	}
}

static void velocitySyncModelRot(GameContext &context) {
	for (auto [entity, vel, body] : context.registry.view<Velocity, RenderBody, tag::VelocitySyncModelRot>().each()) {
		body.rotation = vector3ToRotation(vel.value);
	}
}

void ecs_systems::syncModelRotation(GameContext &context) {
	rotationSyncModel(context);
	aimDirectionSyncModel(context);
	velocitySyncModelRot(context);
}
