#ifndef ENTT_UTILS_HPP
#define ENTT_UTILS_HPP
#include "includes.hpp"
#include "game_context.hpp"

namespace entt_utils {
	entt::entity cloneEntity(entt::registry &src, entt::entity srcEntity, entt::registry &dst);
	bool involvesPlayer(GameContext &context, entt::entity entity);

	template<typename... Cs>
	void assureTypes(entt::registry &reg) {
		(void)std::initializer_list<int>{
			( (void)reg.storage<Cs>(), 0 )...
		};
	}
}

#endif