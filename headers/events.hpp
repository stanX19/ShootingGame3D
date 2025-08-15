#ifndef EVENTS_HPP
#define EVENTS_HPP

#include "includes.hpp"
#include "game_context.hpp"
#include <functional>
#include <vector>

namespace event {
	struct CollisionParty {
		entt::entity id;
		Vector3 pos;			// position during collision
		Vector3 vel;			// actual velocity
	};

	struct CollisionEvent {
		GameContext *context;
		CollisionParty a;
		CollisionParty b;
		float dt;				// frame dt
		float collisionDtRatio;	// collision_dt / frame_dt
	};

	struct KillEvent {
		GameContext *context;
		CollisionParty killer;
		CollisionParty victim;
		Vector3 killLoc;
	};

	struct Listener {
		// CollisionEvent
		void handleCollisionEvent(const CollisionEvent& evt);
		
		// KillEvent
		void handleKillEvent(const KillEvent& evt);
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