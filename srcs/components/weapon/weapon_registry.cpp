#include "classes/weapon_registry.hpp"
#include "weapons.hpp"
#include "basic_utils.hpp"
#include "components/render.hpp"
#include <iostream>
#include <vector>

namespace weapon
{

	void WeaponRegistry::init(const GameConfig &globalCfg)
	{
		registerPredefinedFunctions();
		parseAllWeapons(globalCfg);
	}

	void WeaponRegistry::registerPredefinedFunctions()
	{
		m_predefinedFunctions["basic"] = emplaceWeaponBasic;
		m_predefinedFunctions["sniper"] = emplaceWeaponSniper;
		m_predefinedFunctions["burstSniper"] = emplaceWeaponBurstSniper;
		m_predefinedFunctions["machineGun"] = emplaceWeaponMachineGun;
		m_predefinedFunctions["shotgun"] = emplaceWeaponShotgun;
		m_predefinedFunctions["bigBall"] = emplaceWeaponBigBall;

		m_predefinedFunctions["lazerBasic"] = emplaceWeaponLazerBasic;
		m_predefinedFunctions["lazerMachineGun"] = emplaceWeaponLazerMachineGun;
		m_predefinedFunctions["deletor"] = emplaceWeaponLazerDeletor;
		m_predefinedFunctions["lazerShotgun"] = emplaceWeaponLazerShotgun;

		m_predefinedFunctions["missileBasic"] = emplaceWeaponMissileBasic;
		m_predefinedFunctions["swarm"] = emplaceWeaponMissileSwarm;
		m_predefinedFunctions["torpedo"] = emplaceWeaponMissileTorpedo;
		m_predefinedFunctions["nuke"] = emplaceWeaponMissileNuke;
		m_predefinedFunctions["missileSniper"] = emplaceWeaponMissileSniper;
		m_predefinedFunctions["flares"] = emplaceWeaponMissileFlares;
	}

	nlohmann::json WeaponRegistry::resolveDefinition(
		const std::string &id,
		const nlohmann::json &templates,
		const nlohmann::json &weapons,
		std::unordered_set<std::string> &visited,
		int depth
	) const {
		if (depth > 8 || !visited.insert(id).second) {
			return nlohmann::json::object();
		}

		nlohmann::json raw = nlohmann::json::object();
		if (templates.is_object() && templates.contains(id)) {
			raw = templates[id];
		} else if (weapons.is_object() && weapons.contains(id)) {
			raw = weapons[id];
		} else {
			return nlohmann::json::object();
		}

		nlohmann::json merged = raw;
		if (raw.contains("template") && raw["template"].is_string()) {
			const std::string parentId = raw["template"].get<std::string>();
			nlohmann::json parentResolved = resolveDefinition(parentId, templates, weapons, visited, depth + 1);
			parentResolved.update(merged);
			merged = parentResolved;
		} else if (!raw.contains("template") && id != "bullet" && id != "missile" && id != "lazer") {
			nlohmann::json baseResolved = resolveDefinition("bullet", templates, weapons, visited, depth + 1);
			baseResolved.update(merged);
			merged = baseResolved;
		}

		return merged;
	}

	void WeaponRegistry::parseAllWeapons(const GameConfig &globalCfg)
	{
		const nlohmann::json weaponsSection = globalCfg.getSection("weapons.weapons");
		const nlohmann::json templatesSection = globalCfg.getSection("weapons.templates");
		if (weaponsSection.is_null() || !weaponsSection.is_object())
			return;

		for (auto &[key, value] : weaponsSection.items())
		{
			std::unordered_set<std::string> visited;
			nlohmann::json resolved = resolveDefinition(key, templatesSection, weaponsSection, visited, 0);

			const std::string templateType = resolved.value("template", "bullet");
			const std::string name = resolved.value("name", key);
			const bool isSpecial = resolved.value("isSpecial", false);

			WeaponData data{
				key,
				name,
				templateType,
				isSpecial,
				[resolved](GameContext &context, entt::entity entity, const GameConfig &) {
					emplaceConfiguredWeapon(context, entity, resolved);
				},
				resolved
			};

			m_allWeapons[key] = data;
		}
	}

	std::vector<std::string> WeaponRegistry::getWeaponIdsByType(const std::string &type) const
	{
		std::vector<std::string> ids;
		for (const auto &[id, data] : m_allWeapons)
		{
			if (data.type == type)
			{
				ids.push_back(id);
			}
		}
		return ids;
	}

	std::vector<std::string> WeaponRegistry::getSpecialWeaponIds() const
	{
		std::vector<std::string> ids;
		for (const auto &[id, data] : m_allWeapons)
		{
			if (data.isSpecial)
			{
				ids.push_back(id);
			}
		}
		return ids;
	}

	std::vector<std::string> WeaponRegistry::getStandardWeaponIds() const
	{
		std::vector<std::string> ids;
		for (const auto &[id, data] : m_allWeapons)
		{
			if (!data.isSpecial)
			{
				ids.push_back(id);
			}
		}
		return ids;
	}

	std::string WeaponRegistry::getRandomWeaponId(bool special, int value) const
	{
		std::size_t matchingCount = 0;
		for (const auto &[id, data] : m_allWeapons)
		{
			if (data.isSpecial == special)
				++matchingCount;
		}
		if (matchingCount == 0)
			return {};

		const long long nonNegativeValue = value < 0
			? -static_cast<long long>(value)
			: static_cast<long long>(value);
		const std::size_t selectedIndex =
			static_cast<std::size_t>(nonNegativeValue) % matchingCount;
		std::size_t currentIndex = 0;
		for (const auto &[id, data] : m_allWeapons)
		{
			if (data.isSpecial != special)
				continue;
			if (currentIndex == selectedIndex)
				return id;
			++currentIndex;
		}
		return {};
	}

	std::string WeaponRegistry::getRandomSpecialWeaponId(int value) const
	{
		return getRandomWeaponId(true, value);
	}

	std::string WeaponRegistry::getRandomStandardWeaponId(int value) const
	{
		return getRandomWeaponId(false, value);
	}

	void WeaponRegistry::emplaceRandomWeapon(GameContext &context, entt::entity entity) const
	{
		emplaceRandomWeapon(context, entity, GetRandomValue(0, 100000));
	}

	void WeaponRegistry::emplaceRandomWeapon(GameContext &context, entt::entity entity, int value) const
	{
		const std::string chosenId = getRandomStandardWeaponId(value);
		if (chosenId.empty())
			return;
		emplaceWeaponById(context, entity, chosenId);
	}

	void WeaponRegistry::emplaceRandomSpecialWeapon(GameContext &context, entt::entity entity) const
	{
		emplaceRandomSpecialWeapon(context, entity, GetRandomValue(0, 100000));
	}

	void WeaponRegistry::emplaceRandomSpecialWeapon(GameContext &context, entt::entity entity, int value) const
	{
		const std::string chosenId = getRandomSpecialWeaponId(value);
		if (chosenId.empty())
			return;
		emplaceWeaponById(context, entity, chosenId);
	}

	bool WeaponRegistry::hasWeapon(const std::string& id) const
	{
		return m_allWeapons.find(id) != m_allWeapons.end()
			|| m_predefinedFunctions.find(id) != m_predefinedFunctions.end();
	}

	const WeaponData* WeaponRegistry::getWeaponData(const std::string& id) const
	{
		auto it = m_allWeapons.find(id);
		return (it != m_allWeapons.end()) ? &it->second : nullptr;
	}

	void WeaponRegistry::emplaceWeaponById(GameContext& context, entt::entity entity, const std::string& id) const
	{
		auto it = m_allWeapons.find(id);
		if (it == m_allWeapons.end()) {
			auto predIt = m_predefinedFunctions.find(id);
			if (predIt != m_predefinedFunctions.end()) {
				predIt->second(context, entity, context.config);
			}
			return;
		}

		it->second.emplaceFunc(context, entity, context.config);

		const std::string turretRef = it->second.resolvedJson.value("turretRef", "");
		if (!turretRef.empty() && context.registry.all_of<render::tag::AimDirectionSyncModel>(entity))
		{
			const std::string defaultModel = context.config.getString("turrets.default", "assets/Models/turrets/basic_shooter/turret_basic_shooter.obj");
			const std::string modelPath = context.config.getString("turrets.turrets." + turretRef + ".modelPath", defaultModel);
			auto* renderBody = context.registry.try_get<render::RenderBody>(entity);
			if (renderBody != nullptr)
			{
				renderBody->modelID = context.modelManager.loadModel(modelPath);
			}
		}
	}

} // namespace weapon
