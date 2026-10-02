#ifndef WEAPON_REGISTRY_HPP
#define WEAPON_REGISTRY_HPP

#include "game_config.hpp"
#include <string>

struct GameContext;
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "json.hpp"

namespace weapon {

using WeaponEmplaceFunc = std::function<void(GameContext&, entt::entity, const GameConfig&)>;

struct WeaponData {
    std::string id;
    std::string name;
    std::string type;
    bool isSpecial;
    WeaponEmplaceFunc emplaceFunc;
    nlohmann::json resolvedJson;
};

class WeaponRegistry {
public:
    WeaponRegistry() = default;
    ~WeaponRegistry() = default;

    void init(const GameConfig& globalCfg);

    const std::map<std::string, WeaponData>& getAllWeaponsMap() const { return m_allWeapons; }
    
    bool hasWeapon(const std::string& id) const;
    const WeaponData* getWeaponData(const std::string& id) const;

    // Helpers to get specific lists (e.g. for menus or specific spawners)
    std::vector<std::string> getWeaponIdsByType(const std::string& type) const;
    std::vector<std::string> getSpecialWeaponIds() const;
    std::vector<std::string> getStandardWeaponIds() const;
    std::string getRandomSpecialWeaponId(int value) const;
    std::string getRandomStandardWeaponId(int value) const;

    void emplaceRandomWeapon(GameContext& context, entt::entity entity) const;
    void emplaceRandomWeapon(GameContext& context, entt::entity entity, int value) const;
    void emplaceRandomSpecialWeapon(GameContext& context, entt::entity entity) const;
    void emplaceRandomSpecialWeapon(GameContext& context, entt::entity entity, int value) const;
    void emplaceWeaponById(GameContext& context, entt::entity entity, const std::string& id) const;

private:
    std::map<std::string, WeaponEmplaceFunc> m_predefinedFunctions;
    std::map<std::string, WeaponData> m_allWeapons;

    void registerPredefinedFunctions();
    void parseAllWeapons(const GameConfig& globalCfg);
    nlohmann::json resolveDefinition(
        const std::string &id,
        const nlohmann::json &templates,
        const nlohmann::json &weapons,
        std::unordered_set<std::string> &visited,
        int depth
    ) const;
    std::string getRandomWeaponId(bool special, int value) const;
};

} // namespace weapon

#endif // WEAPON_REGISTRY_HPP
