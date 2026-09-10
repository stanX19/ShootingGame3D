#include "events.hpp"
#include "components/physics.hpp"
#include "components/combat.hpp"
#include "components/render.hpp"
#include "components/effect.hpp"
#include "components/sound.hpp"
#include "components/score.hpp"
#include "components/identity.hpp"
#include "components/weapon.hpp"
#include "entities.hpp"
#include "entt_utils.hpp"

namespace {
	void tryEmitHitSound(GameContext *context, const event::CollisionParty& damager) {
		entt::registry &registry = context->registry;
		
		auto soundPtr = registry.try_get<sound::HitSound>(damager.id);
		if (!soundPtr || soundPtr->id == sound::NONE) return;

		context->dispatcher.enqueue<event::SoundEvent>(event::SoundEvent{
			context,
			soundPtr->id,
			damager.pos,
			soundPtr->volume
		});
	}

	void trySpawnDebris(GameContext *context, const event::CollisionParty& victim, 
	                   const event::CollisionParty& damager) {
		entt::registry &registry = context->registry;
		
		if (!registry.any_of<effect::tag::DropDebris>(victim.id))
			return;

		auto [hpPtr, posPtr, bodyPtr] = registry.try_get<combat::HP, physics::Position, render::RenderBody>(victim.id);
		auto dmgPtr = registry.try_get<combat::Damage>(damager.id);

		if (!hpPtr || hpPtr->value <= 0 || !dmgPtr || dmgPtr->value < 0 || !bodyPtr || !posPtr)
			return;
			
		Vector3 normal = Vector3Normalize(damager.pos - victim.pos);

		float scale = std::cbrt(bodyPtr->scale.x * bodyPtr->scale.y * bodyPtr->scale.z);
		int debrisCount = 2 + static_cast<int>(30 * std::min(1.0f, dmgPtr->value / hpPtr->maxValue));

		Vector3 collisionPos = posPtr->value + normal * scale;
		Vector3 explosionDir = Vector3Normalize(damager.vel) * -50 + victim.vel;
		Color color = ColorLerp(bodyPtr->color, WHITE, 0.5f);

		spawnDebris(*context, collisionPos, scale, color, debrisCount, 5.0f, explosionDir);
	}

	void applyCollisionPhysics(const event::CollisionEvent &evt) {
		const auto& a = evt.a;
		const auto& b = evt.b;
		entt::registry &registry = evt.context->registry;
		
		auto [aMass, aVel, aRot, aHp] = registry.try_get<physics::Mass, physics::Velocity, physics::Rotation, combat::HP>(a.id);
		auto [bMass, bVel, bRot, bHp] = registry.try_get<physics::Mass, physics::Velocity, physics::Rotation, combat::HP>(b.id);
		
		// handle velocity changes
		if (!aMass || !aVel || !bMass || !bVel)
			return;
			
		if (aMass->value <= 0.0f || bMass->value <= 0.0f)
			return;

		bool aIsDead = aHp && aHp->value <= 0.0f;
		bool bIsDead = bHp && bHp->value <= 0.0f;

		if (aIsDead || bIsDead)
			return;
				
		Vector3 normal = Vector3Normalize(b.pos - a.pos);
		Vector3 relativeVelocity = b.vel - a.vel;
		float velAlongNormal = Vector3DotProduct(relativeVelocity, normal);

		if (velAlongNormal >= 0.0f)
			return;

		float collisionElasticity = evt.context->config.physics.collisionElasticity;
		float invMassA = 1.0f / aMass->value;
		float invMassB = 1.0f / bMass->value;
		float impulseScalar = -(1.0f + collisionElasticity) * velAlongNormal;
		impulseScalar /= (invMassA + invMassB);
		Vector3 impulse = normal * impulseScalar;
		
		if (!aIsDead) aVel->value -= impulse * invMassA;
		if (!bIsDead) bVel->value += impulse * invMassB;

		// Handle rotation changes
		if (!aRot || !bRot)
			return;
			
		Vector3 torqueAxis = Vector3CrossProduct(normal, relativeVelocity);
		float torqueMagnitude = Vector3Length(torqueAxis);

		if (torqueMagnitude <= 0.1f)
			return;
		
		float roughness = evt.context->config.physics.roughness;
		float maxKick = evt.context->config.physics.maxAngularKick;

		if (!aIsDead) {
			// A's kick is proportional to B's mass ratio
			float aKick = (bMass->value / aMass->value) * torqueMagnitude * roughness;
			aKick = std::min(aKick, maxKick);
			Quaternion aSpin = QuaternionFromAxisAngle(Vector3Normalize(torqueAxis), aKick);
			aRot->value = QuaternionMultiply(aSpin, aRot->value);
		}

		if (!bIsDead) {
			// B's kick is proportional to A's mass ratio (opposite direction)
			float bKick = (aMass->value / bMass->value) * torqueMagnitude * roughness;
			bKick = std::min(bKick, maxKick);
			Quaternion bSpin = QuaternionFromAxisAngle(Vector3Normalize(torqueAxis), -bKick);
			bRot->value = QuaternionMultiply(bRot->value, bSpin); // bSpin * bRot depending on multiplication order defined
		}
	}

	void recordAttackerContribution(const event::CollisionEvent &evt, entt::entity victimId, entt::entity rootAttacker, float damage) {
		if (victimId == entt::null || !evt.context->registry.valid(victimId))
			return;
		if (rootAttacker == entt::null || damage <= 0.0f)
			return;
		auto &contributors = evt.context->registry.get_or_emplace<combat::DamageContributors>(victimId);
		contributors.recordDamage(rootAttacker, damage, evt.context->gameTime);
	}

	void tryReportPlayerDamage(
		const event::CollisionEvent &evt,
		const event::CollisionParty &victim,
		entt::entity rootAttacker,
		float damage,
		bool isKill
	) {
		if (rootAttacker != evt.context->currentPlayer)
			return;
		if (victim.id == entt::null || !evt.context->registry.valid(victim.id))
			return;
		if (!evt.context->registry.all_of<combat::tag::Targetable>(victim.id))
			return;

		const HitType hitType = isKill ? HitType::KILL : HitType::NORMAL;
		Vector3 targetOffset = Vector3Zeros;
		if (evt.context->registry.all_of<physics::Position>(victim.id)) {
			const Vector3 targetPos = evt.context->registry.get<physics::Position>(victim.id).value;
			targetOffset = Vector3Subtract(victim.pos, targetPos);
		}
		evt.context->hudManager.reportDamage(rootAttacker, victim.id, damage, victim.pos, targetOffset, hitType);
	}

	// assumes killer is eligible to deal damage to victim
	void applyKillerDamageToVictim(
		const event::CollisionEvent &evt,
		const event::CollisionParty &killer,
		const event::CollisionParty &victim
	) {
		entt::registry &registry = evt.context->registry;
		
		combat::Damage *dmgPtr = registry.try_get<combat::Damage>(killer.id);
		auto [shieldPtr, hpPtr] = registry.try_get<combat::EnergyShield, combat::HP>(victim.id);

		if (!dmgPtr || !hpPtr || hpPtr->value <= 0.0f)
			return;

		const float prevHp = hpPtr->value;
		float remainingDmg = dmgPtr->value;

		// Use shield to block if it's an energy weapon
		if (registry.all_of<weapon::tag::Energy>(killer.id) && shieldPtr && shieldPtr->hp > 0) {
			if (shieldPtr->hp > remainingDmg) {  // Can block all damage
				shieldPtr->hp -= remainingDmg;
				shieldPtr->activeTimer = shieldPtr->activeDuration;
				return;
			}
			remainingDmg -= shieldPtr->hp;  // Cannot block all damage
			shieldPtr->hp = 0;
		}

		hpPtr->value -= remainingDmg;

		const bool isKill = (prevHp > 0.0f && hpPtr->value <= 0.0f);
		const entt::entity rootAttacker = entt_utils::getRootScoreParent(evt.context->registry, killer.id);
		recordAttackerContribution(evt, victim.id, rootAttacker, remainingDmg);
		tryReportPlayerDamage(evt, victim, rootAttacker, remainingDmg, isKill);

		const bool isEnergy = registry.all_of<weapon::tag::Energy>(killer.id);
		const Vector3 impactDir = (Vector3LengthSqr(killer.vel) > 0.01f)
			? Vector3Normalize(killer.vel)
			: ((Vector3LengthSqr(Vector3Subtract(victim.pos, killer.pos)) > 0.01f)
				? Vector3Normalize(Vector3Subtract(victim.pos, killer.pos))
				: Vector3{0.0f, 0.0f, 1.0f});
		evt.context->hudManager.reportImpact(victim.id, remainingDmg, impactDir, isEnergy);

		if (isKill) {
			evt.context->dispatcher.enqueue<event::KillEvent>(event::KillEvent{
				evt.context,
				killer,
				victim,
				evt.dt
			});
		}
		
		// Try to spawn debris after applying damage
		trySpawnDebris(evt.context, victim, killer);
	}

	void handleCollisionDamage(const event::CollisionEvent &evt) {
		entt::registry &registry = evt.context->registry;
		combat::HP *aHpPtr = registry.try_get<combat::HP>(evt.a.id);
		combat::HP *bHpPtr = registry.try_get<combat::HP>(evt.b.id);

		bool aWasAlive = !aHpPtr || aHpPtr->value > 0;
		bool bWasAlive = !bHpPtr || bHpPtr->value > 0;

		if (aWasAlive)
			applyKillerDamageToVictim(evt, evt.a, evt.b);
		if (bWasAlive)
			applyKillerDamageToVictim(evt, evt.b, evt.a);
	}
}

void event::Listener::handleCollisionEvent(const CollisionEvent &evt) {
	// std::cout << evt.a.pos.x << " " << evt.b.pos.x << std::endl;
	handleCollisionDamage(evt);

	// apply physics once for the entire event
	applyCollisionPhysics(evt);

	// Emit hit sounds only if collision involves player
	const bool involvesPlayer = entt_utils::getRootScoreParent(evt.context->registry, evt.a.id) == evt.context->currentPlayer ||
		entt_utils::getRootScoreParent(evt.context->registry, evt.b.id) == evt.context->currentPlayer;
	if (!involvesPlayer)
		return;
	tryEmitHitSound(evt.context, evt.a);
	tryEmitHitSound(evt.context, evt.b);
}
