#include "game_hangar.hpp"
#include "engine.hpp"
#include "entities.hpp"

GameHangar::GameHangar(GameContext &context)
	: context(context),
	  renderer(context.mainCamera, context),
	  previewPlayer(entt::null),
	  w1Button("L-Wing 1", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, SKYBLUE, 20),
	  w2Button("R-Wing 1", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, SKYBLUE, 20),
	  w3Button("L-Wing 2", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, SKYBLUE, 20),
	  w4Button("R-Wing 2", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, SKYBLUE, 20),
	  specialButton("Heavy Weapon", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, SKYBLUE, 20),
	  backButton("BACK TO MENU", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, GRAY, 20)
{
	standardWeapons = context.weaponRegistry.getStandardWeaponIds();
	specialWeapons = context.weaponRegistry.getSpecialWeaponIds();
}

GameHangar::~GameHangar() {
	destroyPreviewShip();
}

EngineState GameHangar::run()
{
	spawnPreviewShip();

	float arenaSize = context.config.ARENA_SIZE;
	context.mainCamera.position = Vector3{arenaSize, arenaSize, arenaSize};
	context.mainCamera.target = Vector3{0.0f, 0.0f, 0.0f};
	context.mainCamera.up = Vector3{0.0f, 1.0f, 0.0f};
	context.mainCamera.fovy = 30.0f;
	context.mainCamera.projection = CAMERA_PERSPECTIVE;

	EngineState nextState = EngineState::HANGAR;

	while (!WindowShouldClose() && nextState == EngineState::HANGAR)
	{
		float dt = GetFrameTime();

		float time = (float)GetTime() * 0.025f;
		float dist = 30.0f;
		context.mainCamera.position.x = dist * cosf(time);
		context.mainCamera.position.z = dist * sinf(time);
		context.mainCamera.position.y = 10.0f;
		
		BeginDrawing();
		ClearBackground(BLACK);
		renderer.Render(dt);
		drawUI(nextState);
		EndDrawing();

		inputControls(dt, nextState);
	}

	destroyPreviewShip();

	// Save settings when leaving Hangar
	if (nextState != EngineState::HANGAR) {
		context.config.setString("loadout.w1", context.config.loadout.w1);
		context.config.setString("loadout.w2", context.config.loadout.w2);
		context.config.setString("loadout.w3", context.config.loadout.w3);
		context.config.setString("loadout.w4", context.config.loadout.w4);
		context.config.setString("loadout.special", context.config.loadout.special);
		context.config.save("assets/config/game_config.json");
	}

	return nextState;
}

void GameHangar::cycleWeapon(const std::string& path, std::string &currentWeapon, const std::vector<std::string> &options) {
	if (options.empty()) return;

	std::string newWeapon;
	auto it = std::find(options.begin(), options.end(), currentWeapon);
	if (it == options.end()) {
		newWeapon = options[0];
	} else {
		++it;
		if (it == options.end()) {
			newWeapon = options[0];
		} else {
			newWeapon = *it;
		}
	}
	
	context.config.setString(path, newWeapon);
	spawnPreviewShip();
}



void GameHangar::drawUI(EngineState &nextState)
{
	int screenWidth = GetScreenWidth();
	int screenHeight = GetScreenHeight();

	const char *title = "HANGAR - LOADOUT";
	int titleWidth = MeasureText(title, 40);
	DrawText(title, screenWidth / 2 - titleWidth / 2, 50, 40, SKYBLUE);

	auto drawWeaponButton = [&](ui::TextButtonWidget &widget, const char* path, const char* label, std::string& currentId, const std::vector<std::string>& options, Rectangle bounds) {
		std::string name = "None";
		if (!currentId.empty()) {
			auto map = context.weaponRegistry.getAllWeaponsMap();
			if (map.find(currentId) != map.end()) {
				name = map.at(currentId).name;
			}
		}
		std::string fullText = std::string(label) + ": " + name;
		widget.setBounds(bounds);
		widget.setText(fullText);

		if (widget.tick_and_draw()) {
			cycleWeapon(path, currentId, options);
		}
	};

	float startY = screenHeight - 400.0f;
	drawWeaponButton(w1Button, "loadout.w1", "L-Wing 1", context.config.loadout.w1, standardWeapons, { 50, startY, 400, 50 });
	drawWeaponButton(w2Button, "loadout.w2", "R-Wing 1", context.config.loadout.w2, standardWeapons, { 50, startY + 60, 400, 50 });
	drawWeaponButton(w3Button, "loadout.w3", "L-Wing 2", context.config.loadout.w3, standardWeapons, { 50, startY + 120, 400, 50 });
	drawWeaponButton(w4Button, "loadout.w4", "R-Wing 2", context.config.loadout.w4, standardWeapons, { 50, startY + 180, 400, 50 });
	drawWeaponButton(specialButton, "loadout.special", "Heavy Weapon", context.config.loadout.special, specialWeapons, { 50, startY + 240, 400, 50 });

	Rectangle btnBack = {(float)screenWidth / 2 - 100, (float)screenHeight - 80, 200, 50};
	backButton.setBounds(btnBack);
	if (backButton.tick_and_draw())
	{
		nextState = EngineState::MENU;
	}

	const char *hint = "Press ESC to Return";
	int hintWidth = MeasureText(hint, 20);
	DrawText(hint, screenWidth / 2 - hintWidth / 2, screenHeight - 120, 20, GRAY);
}

void GameHangar::inputControls([[maybe_unused]] float dt, EngineState &nextState)
{
	if (IsKeyPressed(KEY_ESCAPE))
	{
		nextState = EngineState::MENU;
	}
}

void GameHangar::spawnPreviewShip() {
	destroyPreviewShip();
	previewPlayer = spawnPlayer(context, Vector3{0, 0, 0});
	context.currentPlayer = entt::null;
}

void GameHangar::destroyPreviewShip() {
	if (previewPlayer != entt::null && context.registry.valid(previewPlayer)) {
		context.registry.destroy(previewPlayer);
		previewPlayer = entt::null;
	}
}
