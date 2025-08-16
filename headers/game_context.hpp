#ifndef GAME_CONTEXT_HPP
#define GAME_CONTEXT_HPP
#include "includes.hpp"
#include "model_manager.hpp"

struct GameContext {
	ModelManager modelManager;
	entt::registry templateReg;
	entt::registry registry;
	entt::dispatcher dispatcher;
	entt::entity currentPlayer = entt::null;
};

#endif  // GAME_CONTEXT_HPP