#ifndef EVENTS_HPP
#define EVENTS_HPP

#include "includes.hpp"
#include "model_manager.hpp"
#include "game_context.hpp"
#include <functional>
#include <vector>

namespace event {
	struct CollisionEvent {
		GameContext *context;
		entt::entity a;
		entt::entity b;
		float dt;
	};

	struct Listener {
		void handleCollisionPhysics(const CollisionEvent& evt);
		void handleCollisionDamage(const CollisionEvent& evt);
		void handleCollisionFX(const CollisionEvent& evt);
	};
}

namespace event::utils {
	template<typename Event, auto MemberFunc>
	void hookToDispatcher(GameContext& context) {
		static event::Listener listener;
		context.dispatcher.sink<Event>().template connect<MemberFunc>(listener);
	}
	
	void hookAllListeners(GameContext& context);
}



#endif  // EVENTS_HPP