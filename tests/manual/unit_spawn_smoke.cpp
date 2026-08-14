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

	const entt::entity player = spawnPlayer(context, Vector3{0.0f, 0.0f, 0.0f});
	const bool valid = context.registry.valid(player)
		&& context.registry.all_of<AimTarget>(player)
		&& context.registry.all_of<tag::weapon::PlayerControlledFire>(player)
		&& context.registry.all_of<tag::weapon::AIControlledAim>(player);

	context.registry.clear();
	context.soundManager.shutdown();
	context.modelManager.unloadAll();
	CloseWindow();
	return valid ? 0 : 1;
}
