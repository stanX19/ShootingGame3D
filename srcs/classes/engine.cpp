#include "engine.hpp"
#include "settings_menu.hpp"

Engine::Engine() {
	init();
}

Engine::~Engine() {
	shutdown();
}

void Engine::init() {
	InitWindow(1600, 900, "3D Space Shooter");
	SetTargetFPS(60);
	SetExitKey(KEY_NULL);

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
}

void Engine::shutdown() {
	context.soundManager.shutdown();
	context.modelManager.unloadAll();
	CloseWindow();
}

void Engine::run() {
	GameMenu menu(context);
	Game game(context);
	GameHangar hangar(context);
	SettingsMenu settings(context);

	while (state != EngineState::EXIT) {
		if (WindowShouldClose()) {
			state = EngineState::EXIT;
			break;
		}

		switch (state) {
			case EngineState::MENU:
				state = menu.run();
				if (state == EngineState::GAME)
					game.reset();
				break;
			case EngineState::GAME:
				state = game.run();
				break;
			case EngineState::HANGAR:
				state = hangar.run();
				break;
			case EngineState::SETTINGS:
				state = settings.run();
				break;
			case EngineState::EXIT:
				break;
		}
	}
}
