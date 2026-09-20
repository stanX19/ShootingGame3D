#include "events.hpp"
#include "entt_utils.hpp"
#include "components/combat.hpp"
#include "components/score.hpp"
#include "components/identity.hpp"
#include "components/sound.hpp"
#include "components/physics.hpp"
#include "components/collision.hpp"
#include "components/weapon.hpp"
#include "components/faction.hpp"
#include <iostream>

namespace {
	using namespace event;

	void tryEmitDeathSound(const KillEvent& evt) {
		auto soundPtr = evt.context->registry.try_get<sound::DeathSound>(evt.victim.id);
		if (!soundPtr || soundPtr->id == sound::NONE) return;

		evt.context->dispatcher.enqueue<event::SoundEvent>(event::SoundEvent{
			evt.context,
			soundPtr->id,
			evt.victim.pos,
			soundPtr->volume
		});
	}

	void addScore(GameContext &context, entt::entity entity, int score) {
		auto [scorePtr, ownerPtr] = context.registry.try_get<score::Score, identity::Owner>(entity);
		if (scorePtr)
			scorePtr->value += score;
		if (ownerPtr && ownerPtr->root != entity && context.registry.valid(ownerPtr->root))
			addScore(context, ownerPtr->root, score);
	}

	void handleFactionDataUpdate(const KillEvent& evt) {
		if (!evt.context->registry.all_of<identity::tag::Spaceship>(evt.victim.id))
			return;
		auto &factions = evt.context->factions;
		auto killerFacPtr = evt.context->registry.try_get<faction::Faction>(evt.killer.id);
		auto victimFacPtr = evt.context->registry.try_get<faction::Faction>(evt.victim.id);
		auto victimScorePtr = evt.context->registry.try_get<score::KilledScore>(evt.victim.id);

		if (killerFacPtr) {
			auto &killerData = factions[killerFacPtr->value];
			killerData.kills += 1;
			killerData.score += victimScorePtr ? victimScorePtr->value : 0;
		}
		if (victimFacPtr) {
			auto &victimData = factions[victimFacPtr->value];
			victimData.deaths += 1;
		}
	}

	void handleScoreTransfer(const KillEvent& evt) {
		auto victimScorePtr = evt.context->registry.try_get<score::KilledScore>(evt.victim.id);
		if (victimScorePtr)
			addScore(*evt.context, evt.killer.id, victimScorePtr->value);
	}


	void fixVictimPosition(const KillEvent& evt) {
		auto victimPosPtr = evt.context->registry.try_get<physics::Position>(evt.victim.id);
		if (victimPosPtr) {
			victimPosPtr->value = evt.victim.pos;
		}
	}

	void updateVictimVelocity(const KillEvent& evt) {
		if (evt.context->registry.any_of<weapon::tag::Energy>(evt.killer.id))
			return;
		if (evt.context->registry.any_of<weapon::tag::Lazer, weapon::tag::Kinetic>(evt.victim.id))
			return;

		auto [victimVelPtr, victimBodyPtr] = evt.context->registry.try_get<physics::Velocity, collision::CollisionBody>(evt.victim.id);
		collision::CollisionBody* killerBodyPtr = evt.context->registry.try_get<collision::CollisionBody>(evt.killer.id);

		if (victimVelPtr && victimBodyPtr && killerBodyPtr) {
			if (victimBodyPtr->radius * 1.2f < killerBodyPtr->radius) {
				victimVelPtr->value = evt.killer.vel;
			}
		}
	}
	// need a better name for the function below
	void handleVictimPhysics(const KillEvent& evt) {
		fixVictimPosition(evt);
		updateVictimVelocity(evt);
	}

	entt::entity resolveEffectiveKiller(const KillEvent& evt) {
		const entt::entity directKiller = evt.killer.id;
		const auto *killerOwner = evt.context->registry.try_get<identity::Owner>(directKiller);
		const entt::entity rootKiller = killerOwner ? killerOwner->root : directKiller;

		const bool isInvalidKiller = (rootKiller == entt::null || !evt.context->registry.valid(rootKiller));
		const bool isSuicide = (rootKiller == evt.victim.id);
		const bool isAsteroid = (directKiller != entt::null &&
		                         evt.context->registry.valid(directKiller) &&
		                         evt.context->registry.any_of<identity::tag::Asteroid>(directKiller));
		if (!isInvalidKiller && !isSuicide && !isAsteroid)
			return rootKiller;

		const auto *contributors = evt.context->registry.try_get<combat::DamageContributors>(evt.victim.id);
		if (!contributors)
			return rootKiller;

		const float maxAge = evt.context ? evt.context->config.killAttr.fallbackAttributionWindowSeconds : 10.0f;
		const entt::entity fallback = contributors->getLastDamageDealer(evt.context->gameTime, maxAge, evt.victim.id);
		if (fallback != entt::null && evt.context->registry.valid(fallback))
			return fallback;

		return rootKiller;
	}

	bool tryHandlePlayerDeathToast(const KillEvent& evt, entt::entity effectiveKiller) {
		if (evt.victim.id != evt.context->currentPlayer)
			return false;

		const identity::Name *killerName = evt.context->registry.try_get<identity::Name>(effectiveKiller);
		const std::string message = (killerName && effectiveKiller != entt::null)
			? ("Killed by " + killerName->value)
			: "You have been killed";
		evt.context->hudManager.addToastLeftLog(message, RED);
		return true;
	}

	bool isEligibleForAssist(
		const combat::DamageContributors::DamageContributorEntry &entry,
		float maxHp,
		float totalDamage,
		float gameTime,
		const GameConfig::KillAttr &config
	) {
		if ((gameTime - entry.lastHitGameTime) > config.assistWindowSeconds)
			return false;
		if (totalDamage <= 0.0f || entry.damage < config.assistThresholdPct * totalDamage)
			return false;
		if (maxHp > 0.0f && entry.damage < config.assistThresholdPct * maxHp)
			return false;
		return true;
	}

	void tryHandleKillAssistToast(const KillEvent& evt, entt::entity effectiveKiller, const std::string &victimName) {
		const auto *contributors = evt.context->registry.try_get<combat::DamageContributors>(evt.victim.id);
		if (!contributors)
			return;

		const combat::HP *hpPtr = evt.context->registry.try_get<combat::HP>(evt.victim.id);
		const float maxHp = hpPtr ? hpPtr->maxValue : 0.0f;
		const float totalDamage = contributors->getTotalDamage();

		for (size_t i = 0; i < contributors->count; ++i) {
			const auto &entry = contributors->entries[i];
			if (entry.attacker != evt.context->currentPlayer || entry.attacker == effectiveKiller)
				continue;
			if (!isEligibleForAssist(entry, maxHp, totalDamage, evt.context->gameTime, evt.context->config.killAttr))
				continue;

			evt.context->hudManager.addToastLeftLog("Kill assist " + victimName, SKYBLUE);
			return;
		}
	}

	void handleKillToast(const KillEvent& evt) {
		if (!evt.context)
			return;

		const entt::entity effectiveKiller = resolveEffectiveKiller(evt);
		if (tryHandlePlayerDeathToast(evt, effectiveKiller))
			return;

		if (!evt.context->registry.all_of<identity::tag::Spaceship>(evt.victim.id))
			return;

		const identity::Name *victimNamePtr = evt.context->registry.try_get<identity::Name>(evt.victim.id);
		const std::string victimName = victimNamePtr ? victimNamePtr->value : "Enemy";

		if (effectiveKiller == evt.context->currentPlayer) {
			evt.context->hudManager.addToastLeftLog("Killed " + victimName, SKYBLUE);
			return;
		}

		tryHandleKillAssistToast(evt, effectiveKiller, victimName);
	}
}

void event::Listener::handleKillEvent(const KillEvent& evt) {
	tryEmitDeathSound(evt);
	handleFactionDataUpdate(evt);
	handleScoreTransfer(evt);
	handleVictimPhysics(evt);
	handleKillToast(evt);
}