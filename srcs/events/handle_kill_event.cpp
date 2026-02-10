#include "events.hpp"
#include "components.hpp"
#include "components/sound.hpp"
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
		auto [scorePtr, scoreParentPtr] = context.registry.try_get<Score, ScoreParent>(entity);
		if (scorePtr)
			scorePtr->value += score;
		if (scoreParentPtr)
			addScore(context, scoreParentPtr->parent, score);
	}

	void handleFactionDataUpdate(const KillEvent& evt) {
		if (!evt.context->registry.all_of<tag::Spaceship>(evt.victim.id))
			return;
		auto &factions = evt.context->factions;
		auto killerFacPtr = evt.context->registry.try_get<faction::Faction>(evt.killer.id);
		auto victimFacPtr = evt.context->registry.try_get<faction::Faction>(evt.victim.id);
		auto victimScorePtr = evt.context->registry.try_get<KilledScore>(evt.victim.id);

		std::cout << "Entity killed\n";
		if (killerFacPtr) {
			auto &killerData = factions[killerFacPtr->value];
			killerData.kills += 1;
			killerData.score += victimScorePtr ? victimScorePtr->value : 0;
		} else 
			std::cout << "Killer has no faction\n";
		if (victimFacPtr) {
			auto &victimData = factions[victimFacPtr->value];
			victimData.deaths += 1;
		} else 
			std::cout << "Victim has no faction\n";
	}

	void handleScoreTrasfer(const KillEvent& evt) {
		auto victimScorePtr = evt.context->registry.try_get<KilledScore>(evt.victim.id);
		if (victimScorePtr)
			addScore(*evt.context, evt.killer.id, victimScorePtr->value);
	}


	void fixVictimPosition(const KillEvent& evt) {
		auto victimPosPtr = evt.context->registry.try_get<Position>(evt.victim.id);
		if (victimPosPtr) {
			victimPosPtr->value = evt.victim.pos;
		}
	}

	void updateVictimVelocity(const KillEvent& evt) {
		if (evt.context->registry.any_of<tag::bullet_type::Energy>(evt.killer.id))
			return;
		if (evt.context->registry.any_of<tag::bullet_type::Lazer>(evt.victim.id))
			return;

		auto [victimVelPtr, victimBodyPtr] = evt.context->registry.try_get<Velocity, CollisionBody>(evt.victim.id);
		CollisionBody* killerBodyPtr = evt.context->registry.try_get<CollisionBody>(evt.killer.id);

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
}

void event::Listener::handleKillEvent(const KillEvent& evt) {
	tryEmitDeathSound(evt);
	handleFactionDataUpdate(evt);
	handleScoreTrasfer(evt);
	handleVictimPhysics(evt);
}