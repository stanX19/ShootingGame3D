#include "entities.hpp"
#include "entt_utils.hpp"

void weapon::utils::assureBulletTypes(entt::registry &registry) {
	entt_utils::assureTypes<HP, Damage, CollisionBody, RenderBody, Color, Lifespan,
							tag::Bullet, tag::VelocitySyncModelRot, ModelStrech>(registry);
}