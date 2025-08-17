#include "shoot_3d.hpp"
#include "renderer.hpp"
#include <vector>
#include <string>
#include <functional>
#include <cstdio> // Required for std::printf

int main() {
	SetConfigFlags(FLAG_WINDOW_HIDDEN);
	InitWindow(1, 1, "test");
	GameContext context;

	// List of functions to create each weapon
	std::vector<std::function<void(GameContext &, entt::entity)>> weapon_list{
		weapon::emplaceWeaponBasic,
		weapon::emplaceWeaponSniper,
		weapon::emplaceWeaponBurstSniper,
		weapon::emplaceWeaponMachineGun,
		weapon::emplaceWeaponShotgun,
		weapon::emplaceWeaponBigBall,
		weapon::emplaceWeaponLazerBasic,
		weapon::emplaceWeaponLazerMachineGun,
		weapon::emplaceWeaponLazerDeletor,
	};
	
	// Corresponding names for display in the table
	std::vector<std::string> weapon_names{
		"Basic",
		"Sniper",
		"Burst Sniper",
		"Machine Gun",
		"Shotgun",
		"Big Ball",
		"Lazer Basic",
		"Lazer MG",
		"Lazer Deletor",
	};

	// Print the table header with formatted columns
	std::printf("%-4s %-20s %8s %8s %6s %10s %10s %10s %15s\n", 
		"#", "Weapon", "Damage", "Cooldown", "Ammo", "Reload(s)", "AverageDPS", "window Dmg", "Effective DPS");
	std::printf("---- -------------------- -------- -------- ------ ---------- ---------- ---------- ---------------\n");

	int index = 0;
	for (auto func: weapon_list) {
		entt::entity e = context.registry.create();
		func(context, e);
		// --- Fetch Stats ---
		Weapon weapon = context.registry.get<Weapon>(e);
		auto dmg = context.templateReg.get_or_emplace<Damage>(weapon.bulletTemplate, 0.0f).value;
		auto rad = context.registry.get_or_emplace<CollisionBody>(weapon.bulletTemplate, 1.0f).radius;
		const float combatDistance = COMBAT_DIST * 0.7;
		/*
		               |  spreadSin * hypot, assume hypot == comnbat dist
		---------------+
		
		miss rate = collision dist / total dist
		*/
		float hitRate = (rad + 1) / (weapon.bulletData.spreadSin * combatDistance);
		dmg *= std::min(1.0f, hitRate) * weapon.bulletData.bulletCount;
		auto cd = context.registry.get_or_emplace<WeaponCooldown>(e, 0.0f).shootCooldown;
		if (cd == 0)
			cd = 0.0001f;
		auto ammo = context.registry.get_or_emplace<Ammo>(e, 0.0f, 1.0f).maxValue;
		auto reload = context.registry.get_or_emplace<AmmoReload>(e, 1 / cd).value;
		if (reload == 0)
			reload = 0.000001f;
		
		// --- Calculate Derived Stats ---
		// Handle potential division by zero for weapons with no cooldown or reload
		float reloadTime = ammo / reload;
		float burstDmg = dmg * ammo;
		float burstTime = ammo * cd;
		// float burstDps = burstDmg / burstTime;
		float cycleTime = reloadTime > burstTime? reloadTime: burstTime;
		float averageDPS = (cycleTime > 0) ? burstDmg / cycleTime : 0.0f;

		float combatWindow = 2.5f; // seconds
		float time = 0.0f;
		float windowDmg = 0.0f;
		float bullets = ammo;

		while (time < combatWindow) {
			if (bullets > 0) {
				windowDmg += dmg;
				bullets--;
			}
			bullets += reload * cd;  // where cd is dt
			time += cd;
		}
		float effectiveDPS = windowDmg / combatWindow;

		// --- Print the formatted row for the current weapon ---
		std::printf("%-4d %-20s %8.1f %8.2f %6.2f %10.2f %10.1f %10.1f %15.1f\n",
			index,
			weapon_names[index].c_str(),
			dmg,
			cd,
			ammo,
			reloadTime,
			averageDPS,
			windowDmg,
			effectiveDPS
		);

		index++;
	}

	context.modelManager.unloadAll();
	CloseWindow();
	return 0;
}
