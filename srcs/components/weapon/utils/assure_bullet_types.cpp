#include "weapons.hpp"
#include "entt_utils.hpp"
#include "components/sound.hpp"

void weapon::utils::assureBulletTypes(entt::registry &registry) {
	entt_utils::assureTypes<
		HP,
		Damage,
		effect::ExplodeOnDeath,
		effect::InstantDamageOnDeath,
		CollisionBody,
		RenderBody,
		Lifespan,
		tag::Bullet,
		tag::VelocitySyncModelRot,
		tag::bullet_type::Energy,
		tag::bullet_type::Kinetic,
		tag::bullet_type::Lazer,
		tag::Suicidal,
		ModelStrech,
		CollisionBody,
		RenderBody,
		DisappearBound,
		sound::HitSound,
		sound::ShootSound,
		sound::DeathSound,
		SpawnsTrailParticles
	>(registry);
}
