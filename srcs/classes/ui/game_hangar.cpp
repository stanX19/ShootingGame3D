#include "game_hangar.hpp"
#include "engine.hpp"
#include "entities.hpp"
#include "classes/ui/stat_bar.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <stdexcept>

namespace
{
	constexpr float shipPanelY = 100.0f;
	constexpr float shipPanelWidth = 450.0f;
	constexpr float shipPanelHeight = 470.0f;
	constexpr float statRowHeight = 30.0f;
	constexpr float statRowSpacing = 42.0f;

	const struct ShipStatMaximums {
		float hp = 10000.0f;
		float shield = 5000.0f;
		float speed = 160.0f;
		float firepower = std::log2(16.0f + 1.0f); // log2(x+1); Maximum firepower corresponds to x mounts
	} shipStatMaximums;

	float calculateFirepower(std::size_t mountCount)
	{
		return std::log2(static_cast<float>(mountCount) + 1.0f);
	}

	std::vector<ui::StatBar> buildShipStatBars(
		const config::UnitConfig::Definition &definition,
		std::size_t mountCount)
	{
		return {
			ui::StatBar{"HP", definition.stats.hp, shipStatMaximums.hp},
			ui::StatBar{"SHIELD", definition.stats.shield, shipStatMaximums.shield},
			ui::StatBar{"SPEED", definition.stats.maxSpeed, shipStatMaximums.speed},
			ui::StatBar{"FIREPOWER", calculateFirepower(mountCount), shipStatMaximums.firepower}
		};
	}


	Rectangle statRowBounds(float panelX, float statsY, std::size_t index)
	{
		return Rectangle{
			panelX,
			statsY + static_cast<float>(index) * statRowSpacing,
			shipPanelWidth,
			statRowHeight
		};
	}

	std::string displayNameForUnitId(const std::string &id)
	{
		std::string displayName;
		displayName.reserve(id.size() + 4);
		for (std::size_t index = 0; index < id.size(); ++index)
		{
			const unsigned char character = static_cast<unsigned char>(id[index]);
			if (index > 0 && std::isupper(character))
				displayName += ' ';
			displayName += index == 0
							   ? static_cast<char>(std::toupper(character))
							   : static_cast<char>(character);
		}
		return displayName;
	}

	std::string nextWeaponId(
		const std::string &currentWeapon,
		const std::vector<std::string> &options)
	{
		if (options.empty())
			return {};
		const auto current = std::find(options.begin(), options.end(), currentWeapon);
		if (current == options.end() || std::next(current) == options.end())
			return options.front();
		return *std::next(current);
	}

} // namespace

GameHangar::GameHangar(GameContext &context)
  : context(context),
	renderer(context.mainCamera, context),
	previewPlayer(entt::null),
	specialButton("Special Weapon", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, SKYBLUE, 20),
	shipButton("SELECT SHIP", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, SKYBLUE, 20),
	backButton("BACK TO MENU", Rectangle{0.0f, 0.0f, 0.0f, 0.0f}, GRAY, 20)
{
	standardWeapons = context.weaponRegistry.getStandardWeaponIds();
	specialWeapons = context.weaponRegistry.getSpecialWeaponIds();
	shipIds = context.config.units().ids();
	if (shipIds.empty())
		throw std::runtime_error("HANGAR: no unit definitions are available");

	selectedShipId = context.config.getString("loadout.shipId", "");
	if (selectedShipId.empty())
		throw std::invalid_argument("HANGAR: loadout.shipId is required");

	const auto selected = std::find(shipIds.begin(), shipIds.end(), selectedShipId);
	if (selected == shipIds.end())
		throw std::invalid_argument("HANGAR: loadout.shipId references an unknown unit: " + selectedShipId);

	selectedShipIndex = static_cast<std::size_t>(std::distance(shipIds.begin(), selected));
	turretList.setRowUpdate(
		[this](std::size_t index, Rectangle bounds) {
			if (index >= turretButtons.size())
				return false;
			prepareTurretButton(index, bounds);
			if (!turretButtons[index].update())
				return false;
			cycleTurretWeapon(index);
			return true;
		}
	);
	turretList.setRowDraw(
		[this](std::size_t index, Rectangle bounds) {
			if (index >= turretButtons.size())
				return;
			prepareTurretButton(index, bounds);
			turretButtons[index].draw();
		}
	);
}

GameHangar::~GameHangar()
{
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
		const auto &selectedDefinition = context.config.units().get(selectedShipId);
		float dist = 30.0f * selectedDefinition.stats.collisionRadius;
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
	if (nextState != EngineState::HANGAR)
	{
		context.config.setStringArray("loadout.turretWeapons", context.config.loadout.turretWeapons);
		context.config.setString("loadout.specialWeapon", context.config.loadout.specialWeapon);
		context.config.setString("loadout.shipId", selectedShipId);
		context.config.saveChanged();
	}

	return nextState;
}

void GameHangar::cycleWeapon(const std::string &path, std::string &currentWeapon, const std::vector<std::string> &options)
{
	const std::string newWeapon = nextWeaponId(currentWeapon, options);
	if (newWeapon.empty())
		return;
	context.config.setString(path, newWeapon);
	spawnPreviewShip();
}

void GameHangar::cycleTurretWeapon(std::size_t index)
{
	if (index >= context.config.loadout.turretWeapons.size())
		return;
	const std::string newWeapon = nextWeaponId(
		context.config.loadout.turretWeapons[index],
		standardWeapons);
	if (newWeapon.empty())
		return;
	std::vector<std::string> updated = context.config.loadout.turretWeapons;
	updated[index] = newWeapon;
	context.config.setStringArray("loadout.turretWeapons", updated);
	spawnPreviewShip();
}

std::size_t GameHangar::selectedMountCount() const
{
	const auto &definition = context.config.units().get(selectedShipId);
	return context.config.spaceship().get(
													 definition.spaceshipReference)
		.mounts.size();
}

void GameHangar::prepareTurretButton(
	std::size_t index,
	Rectangle bounds)
{
	if (index >= turretButtons.size()
		|| index >= context.config.loadout.turretWeapons.size())
		return;
	const std::string &currentId =
		context.config.loadout.turretWeapons[index];
	std::string name = "None";
	const auto &weapons = context.weaponRegistry.getAllWeaponsMap();
	const auto weapon = weapons.find(currentId);
	if (weapon != weapons.end())
		name = weapon->second.name;
	turretButtons[index].setBounds(bounds);
	turretButtons[index].setText(
		"Weapon " + std::to_string(index + 1) + ": " + name);
}

void GameHangar::resetShipLoadout()
{
	const std::size_t mountCount = selectedMountCount();
	context.config.setStringArray(
		"loadout.turretWeapons",
		std::vector<std::string>(mountCount, "bullet.basic"));
	context.config.setString("loadout.specialWeapon", "missile.basic");
	turretButtons.resize(mountCount);
	turretList.resetScroll();
}

void GameHangar::cycleShip()
{
	if (shipIds.empty())
		return;
	selectedShipIndex = (selectedShipIndex + 1) % shipIds.size();
	selectedShipId = shipIds[selectedShipIndex];
	context.config.setString("loadout.shipId", selectedShipId);
	resetShipLoadout();
	spawnPreviewShip();
}

void GameHangar::drawShipPanel()
{
	if (shipIds.empty())
		return;

	const auto &definition = context.config.units().get(selectedShipId);
	const auto &spaceship = context.config.spaceship().get(definition.spaceshipReference);
	const int screenWidth = GetScreenWidth();
	const float panelX = std::max(20.0f, static_cast<float>(screenWidth) - shipPanelWidth - 40.0f);

	DrawRectangle(
		static_cast<int>(panelX - 20.0f),
		static_cast<int>(shipPanelY - 20.0f),
		static_cast<int>(shipPanelWidth + 40.0f),
		static_cast<int>(shipPanelHeight + 40.0f),
		ColorAlpha(BLUE, 0.25f));
	DrawRectangleLines(
		static_cast<int>(panelX - 20.0f),
		static_cast<int>(shipPanelY - 20.0f),
		static_cast<int>(shipPanelWidth + 40.0f),
		static_cast<int>(shipPanelHeight + 40.0f),
		ColorAlpha(SKYBLUE, 0.7f));

	shipButton.setBounds(Rectangle{
		panelX,
		shipPanelY,
		shipPanelWidth,
		55.0f
	});
	shipButton.setText("SHIP: " + displayNameForUnitId(selectedShipId));
	if (shipButton.tick_and_draw())
	{
		cycleShip();
		return;
	}

	drawShipStats(
		definition,
		spaceship.mounts.size(),
		panelX,
		shipPanelY + 75.0f);
}

void GameHangar::drawShipStats(
	const config::UnitConfig::Definition &definition,
	std::size_t mountCount,
	float panelX,
	float statsY)
{
	const auto statBars = buildShipStatBars(
		definition,
		mountCount
	);
	for (std::size_t index = 0; index < statBars.size(); ++index)
		statBars[index].draw(statRowBounds(panelX, statsY, index));
}

void GameHangar::drawUI(EngineState &nextState)
{
	int screenWidth = GetScreenWidth();
	int screenHeight = GetScreenHeight();

	const char *title = "HANGAR - LOADOUT";
	int titleWidth = MeasureText(title, 40);
	DrawText(title, screenWidth / 2 - titleWidth / 2, 50, 40, SKYBLUE);
	drawShipPanel();

	auto drawWeaponButton = [&](ui::TextButtonWidget &widget,
								const std::string &label,
								const std::string &currentId,
								auto &&onClick,
								Rectangle bounds)
	{
		std::string name = "None";
		if (!currentId.empty())
		{
			const auto &map = context.weaponRegistry.getAllWeaponsMap();
			if (map.find(currentId) != map.end())
			{
				name = map.at(currentId).name;
			}
		}
		std::string fullText = label + ": " + name;
		widget.setBounds(bounds);
		widget.setText(fullText);

		if (widget.tick_and_draw())
			onClick();
	};

	const float leftPaneX = 50.0f;
	const float leftPaneWidth = 400.0f;
	const float leftPaneTop = 100.0f;
	drawWeaponButton(
		specialButton,
		"Special Weapon",
		context.config.loadout.specialWeapon,
		[&]
		{
			cycleWeapon(
				"loadout.specialWeapon",
				context.config.loadout.specialWeapon,
				specialWeapons);
		},
		{leftPaneX, leftPaneTop, leftPaneWidth, 50});

	const Rectangle weaponViewport{
		leftPaneX,
		leftPaneTop + 60.0f,
		leftPaneWidth,
		std::max(120.0f, static_cast<float>(screenHeight) - leftPaneTop - 190.0f)
	};
	const std::size_t mountCount = selectedMountCount();
	turretButtons.resize(mountCount);
	turretList.setBounds(weaponViewport);
	turretList.setItemCount(mountCount);
	turretList.setRowHeight(60.0f);
	turretList.tick_and_draw();

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

void GameHangar::spawnPreviewShip()
{
	destroyPreviewShip();
	previewPlayer = spawnPlayer(context, Vector3{0, 0, 0});
	context.currentPlayer = entt::null;
}

void GameHangar::destroyPreviewShip()
{
	if (previewPlayer == entt::null)
		return;

	std::vector<entt::entity> linkedTurrets;
	for (auto [entity, parent] : context.registry.view<WeaponParent>().each())
	{
		if (parent.parent == previewPlayer)
			linkedTurrets.push_back(entity);
	}
	for (entt::entity turret : linkedTurrets)
	{
		if (context.registry.valid(turret))
			context.registry.destroy(turret);
	}
	if (context.registry.valid(previewPlayer))
		context.registry.destroy(previewPlayer);
	previewPlayer = entt::null;
}
