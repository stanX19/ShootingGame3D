#include "events.hpp"
#include "components.hpp"

namespace {
	void addScore(GameContext &context, entt::entity entity, int score) {
		auto [scorePtr, scoreParentPtr] = context.registry.try_get<Score, ScoreParent>(entity);
		if (scorePtr)
			scorePtr->value += score;
		if (scoreParentPtr)
			addScore(context, scoreParentPtr->parent, score);
	}
}

void event::Listener::handleKillEvent(const KillEvent& evt) {
	// Add score for the kill
	auto victimScorePtr = evt.context->registry.try_get<KilledScore>(evt.victim.id);
	if (victimScorePtr)
		addScore(*evt.context, evt.killer.id, victimScorePtr->value);
	
	auto victimPosPtr = evt.context->registry.try_get<Position>(evt.victim.id);
	if (victimPosPtr) {
		victimPosPtr->value = evt.victim.pos;
	}
}