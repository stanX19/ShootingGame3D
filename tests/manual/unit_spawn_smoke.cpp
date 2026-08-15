#include "shoot_3d.hpp"

int main() {
	SetTraceLogLevel(LOG_ERROR);
	InitWindow(64, 64, "unit spawn smoke");

	GameContext context;
	context.config.init({
		{"audio", "assets/config/audio.json"},
		{"debug", "assets/config/debug.json"},
		{"game", "assets/config/game.json"},
		{"loadout", "assets/config/loadout.json"},
		{"physics", "assets/config/physics.json"},
		{"settings", "assets/config/settings.json"},
		{"sounds", "assets/config/sounds.json"},
		{"units", "assets/config/units.json"},
		{"weapons", "assets/config/weapons.json"},
		{"spaceship", "assets/config/spaceships.json"}
	});
	context.weaponRegistry.init(context.config);
	context.soundManager.init(context.config);
	weapon::utils::setUpRegistry(context);
	event::utils::hookAllListeners(context);
	context.config.setString("loadout.shipId", "fighter");
	context.config.setStringArray(
		"loadout.turretWeapons",
		std::vector<std::string>(4, "bullet.basic")
	);
	context.config.setString("loadout.specialWeapon", "missile.basic");

	const entt::entity player = spawnPlayer(context, Vector3{0.0f, 0.0f, 0.0f});
	const auto* playerBody = context.registry.try_get<RenderBody>(player);
	const auto playerModelPath = playerBody == nullptr
		? std::optional<std::string>{}
		: context.modelManager.getModelPath(playerBody->modelID);
	const bool defaultShipValid = playerModelPath.has_value()
		&& playerModelPath.value() == context.config.spaceship().get(
			"player"
		).modelPath;
	const bool defaultPlayerValid = context.registry.valid(player)
		&& context.registry.all_of<AimTarget>(player)
		&& context.registry.all_of<tag::weapon::PlayerControlledFire>(player)
		&& context.registry.all_of<tag::weapon::AIControlledAim>(player);
	std::size_t defaultTurretCount = 0;
	for (const auto [entity, parent] :
		context.registry.view<WeaponParent>().each()) {
		if (parent.parent == player)
			++defaultTurretCount;
	}
	const bool defaultLoadoutValid = defaultTurretCount == 4
		&& context.config.loadout.turretWeapons.size() == 4;
	context.registry.clear();

	context.config.setString("loadout.shipId", "elite");
	context.config.setStringArray(
		"loadout.turretWeapons",
		std::vector<std::string>(2, "bullet.basic")
	);
	const entt::entity selectedPlayer =
		spawnPlayer(context, Vector3{0.0f, 0.0f, 0.0f});
	const auto* selectedBody = context.registry.try_get<RenderBody>(selectedPlayer);
	const auto selectedModelPath = selectedBody == nullptr
		? std::optional<std::string>{}
		: context.modelManager.getModelPath(selectedBody->modelID);
	const bool selectedShipValid = selectedModelPath.has_value()
		&& selectedModelPath.value() == context.config.spaceship().get(
			"elite"
		).modelPath;
	const bool selectedPlayerValid = context.registry.valid(selectedPlayer)
		&& context.registry.all_of<AimTarget>(selectedPlayer)
		&& context.registry.all_of<tag::weapon::PlayerControlledFire>(selectedPlayer)
		&& context.registry.all_of<tag::weapon::AIControlledAim>(selectedPlayer)
		&& !context.registry.all_of<tag::EliteUnit>(selectedPlayer);
	std::size_t selectedTurretCount = 0;
	for (const auto [entity, parent] :
		context.registry.view<WeaponParent>().each()) {
		if (parent.parent == selectedPlayer)
			++selectedTurretCount;
	}
	const bool selectedLoadoutValid = selectedTurretCount == 2
		&& context.config.loadout.turretWeapons.size() == 2;

	context.registry.clear();
	context.soundManager.shutdown();
	context.modelManager.unloadAll();
	CloseWindow();
	return defaultShipValid
		&& defaultPlayerValid
		&& defaultLoadoutValid
		&& selectedShipValid
		&& selectedPlayerValid
		&& selectedLoadoutValid
		? 0
		: 1;
}
