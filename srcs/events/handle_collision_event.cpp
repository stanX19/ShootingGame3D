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

	void applyCollisionPhysics(const event::CollisionEvent &evt, const event::CollisionParty& victim, 
	                        const event::CollisionParty& killer) {
		entt::registry &registry = evt.context->registry;
		
		auto [vMass, vVel, vRot] = registry.try_get<Mass, Velocity, Rotation>(victim.id);
		auto [kMass, kVel] = registry.try_get<Mass, Velocity>(killer.id);
		
		// Handle velocity change (elastic collision with dampening)
		if (!vMass || !vVel || !kMass || !kVel)
			return;
			
		float totalMass = vMass->value + kMass->value;
		if (totalMass <= 0.0f)
			return;
			
		Vector3 outwardNormal = Vector3Normalize(victim.pos - killer.pos);
		Vector3 relativeVelocity = killer.vel - victim.vel;
		float velAlongNormal = Vector3DotProduct(relativeVelocity, outwardNormal);

		if (velAlongNormal <= 0.0f)
			return; // Moving apart or parallel, no push

		// To prevent both applying force to each other, we can apply standard elastic collision impulse
		// Impulse scalar: j = -(1 + e) * velAlongNormal / (1/m1 + 1/m2)
		float knockbackDampener = evt.context->config.physics.knockbackDampener;
		float restitution = knockbackDampener; // 1.0 = perfectly elastic
		float impulseScalar = (1.0f + restitution) * velAlongNormal;
		impulseScalar /= (1.0f / kMass->value + 1.0f / vMass->value);
		
		Vector3 impulseNormal = outwardNormal * impulseScalar;
		
		vVel->value += impulseNormal / vMass->value;

		// Handle rotation change
		if (!vRot)
			return;
		Vector3 hitToCenter = Vector3Normalize(killer.pos - victim.pos);
		Vector3 impactForcePath = Vector3Normalize(relativeVelocity);
		Vector3 torqueAxis = Vector3CrossProduct(hitToCenter, impactForcePath);

		float torqueMagnitude = Vector3Length(torqueAxis);
		if (torqueMagnitude <= 0.1f)
			return;

		// The angular kick is proportional to mass ratio and impact misalignment
		float angularKickAmount = (kMass->value / vMass->value) * torqueMagnitude * knockbackDampener * evt.context->config.physics.roughness;
		angularKickAmount = std::min(angularKickAmount, evt.context->config.physics.maxAngularKick);

		// Apply torque as a sudden angular rotation change
		Quaternion spin = QuaternionFromAxisAngle(Vector3Normalize(torqueAxis), angularKickAmount);
		vRot->value = QuaternionMultiply(spin, vRot->value);
	}

	void applyDamageToEntity(const event::CollisionEvent &evt, const event::CollisionParty& victim, 
	                        const event::CollisionParty& killer) {
		entt::registry &registry = evt.context->registry;
		
		auto [dmgPtr, killerHpPtr] = registry.try_get<Damage, HP>(killer.id);
		auto [shieldPtr, hpPtr] = registry.try_get<EnergyShield, HP>(victim.id);

		if (!hpPtr || !dmgPtr || hpPtr->value <= 0 || (killerHpPtr && killerHpPtr->value <= 0))
			return;
			
		float dmg = dmgPtr->value;

		// Use shield to block if it's an energy weapon
		if (registry.all_of<tag::bullet_type::Energy>(killer.id) && shieldPtr && shieldPtr->hp > 0) {
			if (shieldPtr->hp > dmg) {  // Can block all damage
				shieldPtr->hp -= dmg;
				shieldPtr->activeTimer = shieldPtr->activeDuration;
				return;
			}
			dmg -= shieldPtr->hp;  // Cannot block all damage
			shieldPtr->hp = 0;
		}

		hpPtr->value -= dmg;

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
}

void event::Listener::handleCollisionEvent(const CollisionEvent &evt) {
	// std::cout << evt.a.pos.x << " " << evt.b.pos.x << std::endl;
	applyDamageToEntity(evt, evt.b, evt.a);
	applyDamageToEntity(evt, evt.a, evt.b);
	applyCollisionPhysics(evt, evt.b, evt.a);
	applyCollisionPhysics(evt, evt.a, evt.b);

	// Emit hit sounds only if collision involves player
	if (!entt_utils::involvesPlayer(*evt.context, evt.a.id) && !entt_utils::involvesPlayer(*evt.context, evt.b.id))
		return;
	tryEmitHitSound(evt.context, evt.a);
	tryEmitHitSound(evt.context, evt.b);
}