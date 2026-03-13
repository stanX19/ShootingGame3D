#ifndef WEAPON_REGISTRY_HPP
#define WEAPON_REGISTRY_HPP

#include "game_config.hpp"
#include <string>

struct GameContext;
#include <vector>
#include <map>
#include <functional>

namespace weapon {

using WeaponEmplaceFunc = std::function<void(GameContext&, entt::entity, const GameConfig&)>;

struct WeaponData {
    std::string id;
    std::string name;
    std::string type;
    bool isSpecial;
    WeaponEmplaceFunc emplaceFunc;
};

class WeaponRegistry {
public:
    WeaponRegistry() = default;
    ~WeaponRegistry() = default;

    void init(const GameConfig& globalCfg);

    const std::map<std::string, WeaponData>& getAllWeaponsMap() const { return allWeapons; }
    
    // Helpers to get specific lists (e.g. for menus or specific spawners)
    std::vector<std::string> getWeaponIdsByType(const std::string& type) const;
    std::vector<std::string> getSpecialWeaponIds() const;
    std::vector<std::string> getStandardWeaponIds() const;

    void emplaceRandomAttackWeapon(GameContext& context, entt::entity entity) const;
    void emplaceRandomAttackWeapon(GameContext& context, entt::entity entity, int value) const;
    void emplaceRandomMissileWeapon(GameContext& context, entt::entity entity) const;
    void emplaceRandomMissileWeapon(GameContext& context, entt::entity entity, int value) const;

private:
    std::map<std::string, WeaponEmplaceFunc> predefinedFunctions;
    std::map<std::string, WeaponData> allWeapons;

    void registerPredefinedFunctions();
    void parseWeaponsOfType(const GameConfig& globalCfg, const std::string& category);
};

} // namespace weapon

#endif // WEAPON_REGISTRY_HPP
