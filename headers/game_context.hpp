#ifndef GAME_CONTEXT_HPP
#define GAME_CONTEXT_HPP
#include "includes.hpp"
#include "game_config.hpp"
#include "model_manager.hpp"
#include "collision_body_manager.hpp"
#include "sound_manager.hpp"
#include "factions.hpp"
#include "classes/weapon_registry.hpp"
#include "classes/hud_manager.hpp"
#include <map>

struct FactionData {
	int score;
	int kills;
	int deaths;
};

struct GameContext {
	GameConfig config;
	ModelManager modelManager;
	CollisionBodyManager collisionBodyManager;
	SoundManager soundManager;
	weapon::WeaponRegistry weaponRegistry;
	HudManager hudManager;
	entt::registry templateReg;
	entt::registry registry;
	entt::dispatcher dispatcher;
	entt::entity currentPlayer = entt::null;
	Camera3D mainCamera;
	std::map<faction::FacVal, FactionData> factions;
	float gameTime = 0.0f;
};

#endif  // GAME_CONTEXT_HPP
