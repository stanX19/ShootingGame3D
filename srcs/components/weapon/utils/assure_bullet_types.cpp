#include "weapons.hpp"
#include "entt_utils.hpp"
#include "components/sound.hpp"
#include "components/physics.hpp"
#include "components/spaceship.hpp"
#include "components/faction.hpp"
#include "components/combat.hpp"
#include "components/effect.hpp"
#include "components/render.hpp"

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
		lifetime::DisappearBound,
		sound::HitSound,
		sound::ShootSound,
		sound::DeathSound,
		effect::HasSimpleTrail,
		effect::HasMultiTrail,
		physics::Rotation,
		physics::RotationVelocity,
		physics::tag::VelocitySyncRot,
		spaceship::MoveTarget,
		spaceship::tag::AIMoveControl,
		weapon::tag::Missile,
		physics::Mass,
		spaceship::TurnSpeed,
		physics::ScalarAcceleration,
		combat::DelayedDamage,
		combat::tag::Targetable,
		faction::Faction
	>(registry);
}
