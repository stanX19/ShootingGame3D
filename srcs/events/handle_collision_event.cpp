#include "events.hpp"
#include "components.hpp"
#include "entities.hpp"
#include "components/sound.hpp"
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
		
		if (!registry.any_of<tag::effect::DropDebris>(victim.id))
			return;

		auto [hpPtr, posPtr, bodyPtr] = registry.try_get<HP, Position, RenderBody>(victim.id);
		auto dmgPtr = registry.try_get<Damage>(damager.id);

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
		
		auto [aMass, aVel, aRot, aHp] = registry.try_get<Mass, Velocity, Rotation, HP>(a.id);
		auto [bMass, bVel, bRot, bHp] = registry.try_get<Mass, Velocity, Rotation, HP>(b.id);
		
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

	// assumes killer is eligible to deal damage to victim
	void applyKillerDamageToVictim(
		const event::CollisionEvent &evt,
		const event::CollisionParty &killer,
		const event::CollisionParty &victim
	) {
		entt::registry &registry = evt.context->registry;
		
		Damage *dmgPtr = registry.try_get<Damage>(killer.id);
		auto [shieldPtr, hpPtr] = registry.try_get<EnergyShield, HP>(victim.id);

		if (!dmgPtr || !hpPtr || hpPtr->value <= 0)
			return;
			
		float remainingDmg = dmgPtr->value;

		// Use shield to block if it's an energy weapon
		if (registry.all_of<tag::bullet_type::Energy>(killer.id) && shieldPtr && shieldPtr->hp > 0) {
			if (shieldPtr->hp > remainingDmg) {  // Can block all damage
				shieldPtr->hp -= remainingDmg;
				shieldPtr->activeTimer = shieldPtr->activeDuration;
				return;
			}
			remainingDmg -= shieldPtr->hp;  // Cannot block all damage
			shieldPtr->hp = 0;
		}

		hpPtr->value -= remainingDmg;

		if (hpPtr->value < 0) {
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
		HP *aHpPtr = registry.try_get<HP>(evt.a.id);
		HP *bHpPtr = registry.try_get<HP>(evt.b.id);

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
	if (!entt_utils::involvesPlayer(*evt.context, evt.a.id) && !entt_utils::involvesPlayer(*evt.context, evt.b.id))
		return;
	tryEmitHitSound(evt.context, evt.a);
	tryEmitHitSound(evt.context, evt.b);
}
