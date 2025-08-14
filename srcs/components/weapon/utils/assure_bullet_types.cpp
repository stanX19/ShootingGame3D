#include "weapons.hpp"
#include "entt_utils.hpp"

void weapon::utils::assureBulletTypes(entt::registry &registry) {
	entt_utils::assureTypes<HP, Damage, CollisionBody, RenderBody, Lifespan,
							tag::Bullet, tag::VelocitySyncModelRot,
							tag::bullet_type::Energy, tag::bullet_type::Kinetic,
							ModelStrech>(registry);
}