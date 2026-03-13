#include "classes/weapon_registry.hpp"
#include "weapons.hpp"
#include "basic_utils.hpp"
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
		predefinedFunctions["bullet.basic"] = emplaceWeaponBasic;
		predefinedFunctions["bullet.sniper"] = emplaceWeaponSniper;
		predefinedFunctions["bullet.burstSniper"] = emplaceWeaponBurstSniper;
		predefinedFunctions["bullet.machineGun"] = emplaceWeaponMachineGun;
		predefinedFunctions["bullet.shotgun"] = emplaceWeaponShotgun;
		predefinedFunctions["bullet.bigBall"] = emplaceWeaponBigBall;

		predefinedFunctions["lazer.basic"] = emplaceWeaponLazerBasic;
		predefinedFunctions["lazer.machineGun"] = emplaceWeaponLazerMachineGun;
		predefinedFunctions["lazer.deletor"] = emplaceWeaponLazerDeletor;
		predefinedFunctions["lazer.shotgun"] = emplaceWeaponLazerShotgun;

		predefinedFunctions["missile.basic"] = emplaceWeaponMissileBasic;
		predefinedFunctions["missile.swarm"] = emplaceWeaponMissileSwarm;
		predefinedFunctions["missile.torpedo"] = emplaceWeaponMissileTorpedo;
		predefinedFunctions["missile.nuke"] = emplaceWeaponMissileNuke;
		predefinedFunctions["missile.sniper"] = emplaceWeaponMissileSniper;
		predefinedFunctions["missile.flares"] = emplaceWeaponMissileFlares;
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
			auto it = predefinedFunctions.find(id);
			if (it != predefinedFunctions.end())
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

			allWeapons[id] = {id, name, category, isSpecial, func};
		}
	}

	std::vector<std::string> WeaponRegistry::getWeaponIdsByType(const std::string &type) const
	{
		std::vector<std::string> ids;
		for (const auto &[id, data] : allWeapons)
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
		for (const auto &[id, data] : allWeapons)
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
		for (const auto &[id, data] : allWeapons)
		{
			if (!data.isSpecial)
			{
				ids.push_back(id);
			}
		}
		return ids;
	}

	void WeaponRegistry::emplaceRandomWeapon(GameContext &context, entt::entity entity) const
	{
		emplaceRandomWeapon(context, entity, GetRandomValue(0, 100000));
	}

	void WeaponRegistry::emplaceRandomWeapon(GameContext &context, entt::entity entity, int value) const
	{
		std::vector<std::string> candidates = getStandardWeaponIds();
		if (candidates.empty())
			return;
		const std::string &chosenId = candidates[value % candidates.size()];
		const SubGameConfig subCfg = context.config.getSubConfig("weapons." + allWeapons.at(chosenId).type + ".weapons." + chosenId.substr(chosenId.find('.') + 1));
		allWeapons.at(chosenId).emplaceFunc(context, entity, subCfg);
	}

	void WeaponRegistry::emplaceRandomSpecialWeapon(GameContext &context, entt::entity entity) const
	{
		emplaceRandomSpecialWeapon(context, entity, GetRandomValue(0, 100000));
	}

	void WeaponRegistry::emplaceRandomSpecialWeapon(GameContext &context, entt::entity entity, int value) const
	{
		std::vector<std::string> candidates = getSpecialWeaponIds();
		if (candidates.empty())
			return;
		const std::string &chosenId = candidates[value % candidates.size()];
		const SubGameConfig subCfg = context.config.getSubConfig("weapons." + allWeapons.at(chosenId).type + ".weapons." + chosenId.substr(chosenId.find('.') + 1));
		allWeapons.at(chosenId).emplaceFunc(context, entity, subCfg);
	}

} // namespace weapon
