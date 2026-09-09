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

	m_context.config.init({
		{"audio", "assets/config/audio.json"},
		{"debug", "assets/config/debug.json"},
		{"game", "assets/config/game.json"},
		{"hud", "assets/config/hud.json"},
		{"loadout", "assets/config/loadout.json"},
		{"physics", "assets/config/physics.json"},
		{"settings", "assets/config/settings.json"},
		{"sounds", "assets/config/sounds.json"},
		{"units", "assets/config/units.json"},
		{"weapons", "assets/config/weapons.json"},
		{"spaceship", "assets/config/spaceships.json"}
	});
	m_context.weaponRegistry.init(m_context.config);
	m_context.soundManager.init(m_context.config);
}

void Engine::shutdown() {
	m_context.soundManager.shutdown();
	m_context.modelManager.unloadAll();
	CloseWindow();
}

void Engine::run() {
	GameMenu menu(m_context);
	Game game(m_context);
	GameHangar hangar(m_context);
	SettingsMenu settings(m_context);

	while (m_state != EngineState::EXIT) {
		if (WindowShouldClose()) {
			m_state = EngineState::EXIT;
			break;
		}

		switch (m_state) {
			case EngineState::MENU:
				m_state = menu.run();
				if (m_state == EngineState::GAME)
					game.reset();
				break;
			case EngineState::GAME:
				m_state = game.run();
				break;
			case EngineState::HANGAR:
				m_state = hangar.run();
				break;
			case EngineState::SETTINGS:
				m_state = settings.run();
				break;
			case EngineState::EXIT:
				break;
		}
	}
}
