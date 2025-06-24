#ifndef GAME_CONTEXT_HPP
#define GAME_CONTEXT_HPP
#include "includes.hpp"
#include "mesh_manager.hpp"

struct GameContext {
	MeshManager meshManager;
	entt::registry registry;
};

#endif  // GAME_CONTEXT_HPP