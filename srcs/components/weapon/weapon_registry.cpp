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

		parseWeaponsOfType(globalCfg, "bullet");
		parseWeaponsOfType(globalCfg, "lazer");
		parseWeaponsOfType(globalCfg, "missile");
	}

	void WeaponRegistry::registerPredefinedFunctions()
	{
		m_predefinedFunctions["bullet.basic"] = emplaceWeaponBasic;
		m_predefinedFunctions["bullet.sniper"] = emplaceWeaponSniper;
		m_predefinedFunctions["bullet.burstSniper"] = emplaceWeaponBurstSniper;
		m_predefinedFunctions["bullet.machineGun"] = emplaceWeaponMachineGun;
		m_predefinedFunctions["bullet.shotgun"] = emplaceWeaponShotgun;
		m_predefinedFunctions["bullet.bigBall"] = emplaceWeaponBigBall;

		m_predefinedFunctions["lazer.basic"] = emplaceWeaponLazerBasic;
		m_predefinedFunctions["lazer.machineGun"] = emplaceWeaponLazerMachineGun;
		m_predefinedFunctions["lazer.deletor"] = emplaceWeaponLazerDeletor;
		m_predefinedFunctions["lazer.shotgun"] = emplaceWeaponLazerShotgun;

		m_predefinedFunctions["missile.basic"] = emplaceWeaponMissileBasic;
		m_predefinedFunctions["missile.swarm"] = emplaceWeaponMissileSwarm;
		m_predefinedFunctions["missile.torpedo"] = emplaceWeaponMissileTorpedo;
		m_predefinedFunctions["missile.nuke"] = emplaceWeaponMissileNuke;
		m_predefinedFunctions["missile.sniper"] = emplaceWeaponMissileSniper;
		m_predefinedFunctions["missile.flares"] = emplaceWeaponMissileFlares;
	}

	void WeaponRegistry::parseWeaponsOfType(const GameConfig &globalCfg, const std::string &category)
	{
		nlohmann::json section = globalCfg.getSection("weapons." + category + ".weapons");
		if (section.is_null() || !section.is_object())
			return;

		for (auto &[key, value] : section.items())
		{
			std::string id = category + "." + key;

			SubGameConfig subCfg = globalCfg.getSubConfig("weapons." + category + ".weapons." + key);
			std::string name = subCfg.getString("name", "Unknown " + category);
			bool isSpecial = subCfg.getBool("isSpecial", false);

			WeaponEmplaceFunc func;
			auto it = m_predefinedFunctions.find(id);
			if (it != m_predefinedFunctions.end())
			{
				func = it->second;
			}
			else
			{
				if (category == "bullet")
					func = emplaceGenericBullet;
				else if (category == "lazer")
					func = emplaceGenericLazer;
				else if (category == "missile")
					func = emplaceGenericMissile;
			}

			m_allWeapons[id] = {id, name, category, isSpecial, func};
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

	void WeaponRegistry::emplaceWeaponById(GameContext& context, entt::entity entity, const std::string& id) const
	{
		auto it = m_allWeapons.find(id);
		if (it != m_allWeapons.end())
		{
			const std::string type = it->second.type;
			const std::string subId = id.substr(id.find('.') + 1);
			const SubGameConfig subCfg = context.config.getSubConfig("weapons." + type + ".weapons." + subId);
			it->second.emplaceFunc(context, entity, subCfg);

			const std::string turretRef = subCfg.getString("turretRef", "");
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
	}

} // namespace weapon
