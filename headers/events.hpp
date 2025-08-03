#ifndef EVENTS_HPP
#define EVENTS_HPP

#include "includes.hpp"
#include "game_context.hpp"
#include <functional>
#include <vector>

namespace event {
	struct CollisionEvent {
		GameContext *context;
		entt::entity a;
		Vector3 posA;			// position during collision
		Vector3 velA;			// actual velocity
		entt::entity b;
		Vector3 posB;			// same
		Vector3 velB;
		float dt;				// frame dt
		float collisionDtRatio;	// collision_dt / frame_dt
	};

	struct KillEvent {
		GameContext *context;
		entt::entity killer;
		entt::entity victim;
	};

	struct Listener {
		// CollisionEvent
		void handleCollisionPhysics(const CollisionEvent& evt);
		void handleCollisionDamage(const CollisionEvent& evt);
		void handleCollisionFX(const CollisionEvent& evt);
		
		// KillEvent
		void handleKillingScore(const KillEvent& evt);
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