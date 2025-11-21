#include "events.hpp"
#include "components.hpp"

namespace {
	using namespace event;

	void addScore(GameContext &context, entt::entity entity, int score) {
		auto [scorePtr, scoreParentPtr] = context.registry.try_get<Score, ScoreParent>(entity);
		if (scorePtr)
			scorePtr->value += score;
		if (scoreParentPtr)
			addScore(context, scoreParentPtr->parent, score);
	}

	void handleScoreTrasfer(const KillEvent& evt) {
		auto victimScorePtr = evt.context->registry.try_get<KilledScore>(evt.victim.id);
		if (victimScorePtr)
			addScore(*evt.context, evt.killer.id, victimScorePtr->value);
	}

	// need a better name for the function below
	void handleVictimPhysics(const KillEvent& evt) {
		if (evt.context->registry.any_of<tag::bullet_type::Energy>(evt.killer.id))
			return;
		if (evt.context->registry.any_of<tag::bullet_type::Lazer>(evt.victim.id))
			return;

		auto victimPosPtr = evt.context->registry.try_get<Position>(evt.victim.id);
		if (victimPosPtr) {
			victimPosPtr->value = evt.victim.pos;
		}
		auto [victimVelPtr, victimBodyPtr] = evt.context->registry.try_get<Velocity, CollisionBody>(evt.victim.id);
		CollisionBody* killerBodyPtr = evt.context->registry.try_get<CollisionBody>(evt.killer.id);

		if (victimVelPtr && victimBodyPtr && killerBodyPtr) {
			if (victimBodyPtr->radius * 1.2f < killerBodyPtr->radius) {
				victimVelPtr->value = evt.killer.vel;
			}
		}
	}
}

void event::Listener::handleKillEvent(const KillEvent& evt) {
	handleScoreTrasfer(evt);
	handleVictimPhysics(evt);
}