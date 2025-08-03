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

void event::Listener::handleKillingScore(const KillEvent& evt) {
	auto victimScorePtr = evt.context->registry.try_get<KilledScore>(evt.victim);
	if (victimScorePtr)
		addScore(*evt.context, evt.killer, victimScorePtr->value);
}
