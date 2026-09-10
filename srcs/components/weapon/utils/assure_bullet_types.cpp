#include "weapons.hpp"
#include "entt_utils.hpp"
#include "components/sound.hpp"

void weapon::utils::assureBulletTypes(entt::registry &registry) {
	entt_utils::assureTypes<
		combat::HP,
		combat::Damage,
		effect::ExplodeOnDeath,
		effect::InstantDamageOnDeath,
		collision::CollisionBody,
		render::RenderBody,
		lifetime::Lifespan,
		weapon::tag::Bullet,
		render::tag::VelocitySyncModelRot,
		weapon::tag::Energy,
		weapon::tag::Kinetic,
		weapon::tag::Lazer,
		spaceship::tag::Suicidal,
		render::ModelStrech,
		collision::CollisionBody,
		render::RenderBody,
		lifetime::DisappearBound,
		sound::HitSound,
		sound::ShootSound,
		sound::DeathSound,
		effect::SpawnsTrailParticles
	>(registry);
}
