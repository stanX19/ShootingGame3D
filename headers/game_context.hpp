#ifndef GAME_CONTEXT_HPP
#define GAME_CONTEXT_HPP
#include "includes.hpp"
#include "model_manager.hpp"

struct GameContext {
	ModelManager meshManager;
	entt::registry registry;
};

#endif  // GAME_CONTEXT_HPP