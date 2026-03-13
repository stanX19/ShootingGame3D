#ifndef WEAPON_REGISTRY_HPP
#define WEAPON_REGISTRY_HPP

#include "game_context.hpp"
#include <string>
#include <vector>
#include <functional>

namespace weapon {

struct WeaponDescriptor {
    std::string name;
    std::string category;
    std::function<void(GameContext&, entt::entity)> emplaceFunc;
};

const std::vector<WeaponDescriptor>& getAllWeapons();
void emplaceRandomAttackWeapon(GameContext &context, entt::entity entity);
void emplaceRandomAttackWeapon(GameContext &context, entt::entity entity, int value);
void emplaceRandomMissileWeapon(GameContext &context, entt::entity entity);
void emplaceRandomMissileWeapon(GameContext &context, entt::entity entity, int value);

} // namespace weapon

#endif // WEAPON_REGISTRY_HPP
