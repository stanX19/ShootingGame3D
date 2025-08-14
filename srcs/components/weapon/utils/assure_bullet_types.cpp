#include "weapons.hpp"
#include "entt_utils.hpp"

void weapon::utils::assureBulletTypes(entt::registry &registry) {
	entt_utils::assureTypes<HP, Damage, CollisionBody, RenderBody, Lifespan,
							tag::Bullet, tag::VelocitySyncModelRot, ModelStrech>(registry);
}