#include "renderer.hpp"
#include <iostream>
#include <algorithm>

Renderer::Renderer(Camera3D &cam, GameContext &context)
	: camera(cam), context(context)
{
	LoadDefaultShader();
	LoadShaderWithFallback();
	SetupShaderUniforms();
}

Renderer::~Renderer()
{
	if (shader.id != 0)
	{
		// UnloadShader(shader);  // this seg faults idk why
		shader = {0, NULL};
	}
}

void Renderer::LoadDefaultShader()
{
	defaultShader = LoadShader(NULL, NULL);
}

void Renderer::LoadShaderWithFallback()
{
	shader = LoadShader("shaders/sunlight.vs", "shaders/sunlight.fs");
	if (shader.id == 0)
	{
		TraceLog(LOG_WARNING, "Custom shader failed to load. Using default shader.");
		shader = LoadShader(NULL, NULL);
		return ;
	}

	Mesh sphereMesh = GenMeshSphere(1.0f, 64, 64);
	sphereModel = LoadModelFromMesh(sphereMesh);
	sphereModel.materials[0].shader = shader;
	
}

void Renderer::SetupShaderUniforms()
{
	lightPosLoc = GetShaderLocation(shader, "lightPosition");
	lightColorLoc = GetShaderLocation(shader, "lightColor");

	Vector3 lightPos = { 100000, 100000, 100000 };
	SetShaderValue(shader, lightPosLoc, &lightPos, SHADER_UNIFORM_VEC3);

	Vector3 lightColor = { 1.0f, 1.0f, 1.0f };
	SetShaderValue(shader, lightColorLoc, &lightColor, SHADER_UNIFORM_VEC3);
}

void Renderer::Render()
{
	BeginDrawing();
	ClearBackground(BLACK);

	BeginMode3D(camera);
	// DrawGrid(ARENA_SIZE * 2 / 10 + 1, 10);

	HandleLightSource();
	DrawEntitiesWithoutShader();
	DrawEntitiesWithShader();

	EndMode3D();

	DrawHealthBars();
	DrawTargetable();

	// HUD
	DrawHUD();
	DrawFPS(10, 10);

	DrawTexts();

	EndDrawing();
}

void Renderer::DrawTexts() {
	auto playerView = context.registry.view<tag::Player>();

	if (playerView.begin() != playerView.end())
	{
		int totalEntities = 0;
		auto hittableView = context.registry.view<CollisionBody, Position, HP>();
		for (auto entity : hittableView)
		{
			if (hittableView.get<HP>(entity).value > 0)
			{
				totalEntities++;
			}
		}
		DrawText(TextFormat("Entities: %d", totalEntities), 10, 30, 20, WHITE);
		DrawText("WASD to move, Arrows to turn, LMB/Space to shoot", 10, 70, 20, WHITE);
	}
	else
	{
		const char *msg = "GAME OVER - PRESS R TO RESTART";
		int w = MeasureText(msg, 40);
		DrawText(msg, GetScreenWidth() / 2 - w / 2, GetScreenHeight() / 2, 40, RED);
	}
}

void Renderer::HandleLightSource()
{
	auto view = context.registry.view<Position, RenderBody, tag::LightSource>();

	for (auto entity : view)
	{
		const Position &pos = view.get<Position>(entity);
		const RenderBody &body = view.get<RenderBody>(entity);

		Vector3 color = {body.color.r / 255.0f, body.color.g / 255.0f, body.color.b / 255.0f};
		SetShaderValue(shader, lightPosLoc, &pos.value, SHADER_UNIFORM_VEC3);
		SetShaderValue(shader, lightColorLoc, &color, SHADER_UNIFORM_VEC3);
		break ;
	}
}


// if (context.registry.all_of<Rotation>(entity))
// {
// 	auto &rot = context.registry.get<Rotation>(entity);
// 	Vector3 forward = GetForwardVector(rot);
// 	Vector3 end = pos.value + forward * (body.radius * 100);
// 	DrawLine3D(pos.value, end, WHITE);
// 	end = pos.value + GetUpVector(rot) * (body.radius * 10);
// 	DrawLine3D(pos.value, end, GREEN);
// }

void Renderer::DrawEntityModel(const Position &pos, const RenderBody &body)
{
	Model &model = context.meshManager.getModel(body.modelID);

	Vector3 scale = { body.scale, body.scale, body.scale };
	Vector3 axis;
	float angle;
	QuaternionToAxisAngle(body.rotation, &axis, &angle);
	// std::cout << "Entity rotation axis: (" << axis.x << ", " << axis.y << ", " << axis.z
	//   << "), angle: " << RAD2DEG * angle << " deg" << std::endl;
	DrawModelEx(model, pos.value + body.translation, axis, angle * RAD2DEG, scale, body.color);
}

void Renderer::DrawEntitiesWithoutShader()
{
	auto view = context.registry.view<Position, RenderBody>(entt::exclude<tag::Shaded>);

	for (auto entity : view)
	{
		const Position &pos = view.get<Position>(entity);
		const RenderBody &body = view.get<RenderBody>(entity);
		context.meshManager.getModel(body.modelID).materials[0].shader = defaultShader;
		
		DrawEntityModel(pos, body);
	}
}

void Renderer::DrawEntitiesWithShader()
{
	BeginShaderMode(shader);

	auto view = context.registry.view<Position, RenderBody, tag::Shaded>();
	for (auto entity : view)
	{
		const Position &pos = view.get<Position>(entity);
		const RenderBody &body = view.get<RenderBody>(entity);
		context.meshManager.getModel(body.modelID).materials[0].shader = shader;

		DrawEntityModel(pos, body);
	}

	EndShaderMode();
}

bool isInFrontOfCamera(const Vector3 &entityPos, const Camera3D &camera)
{
	Vector3 cameraToEntity = entityPos - camera.position;
	Vector3 forward = camera.target - camera.position;
	return Vector3DotProduct(cameraToEntity, forward) > 0;
}

void Renderer::DrawHealthBars()
{
	auto view = context.registry.view<Position, CollisionBody, HP>();
	for (auto entity : view)
	{
		auto &pos = view.get<Position>(entity);
		auto &hp = view.get<HP>(entity);

		if (hp.value == hp.maxValue)
			continue;
		if (!isInFrontOfCamera(pos.value, camera))
			continue;
		Vector2 screen = GetWorldToScreen(pos.value, camera);

		screen.y -= 20;

		if (screen.x < 0 || screen.x > GetScreenWidth() ||
			screen.y < 0 || screen.y > GetScreenHeight())
			continue;

		float w = 40, h = 4;
		float pct = (float)hp.value / hp.maxValue;

		DrawRectangle(screen.x - w / 2, screen.y, w, h, GRAY);
		DrawRectangle(screen.x - w / 2, screen.y, w * pct, h, GREEN);
	}
}

void Renderer::DrawTargetable()
{
	auto view = context.registry.view<Position, PlayerTargetable>();
	auto playerView = context.registry.view<tag::Player, AimTarget>();

	entt::entity targetedEntity = playerView.begin() != playerView.end() ? playerView.get<AimTarget>(*playerView.begin()).entity : entt::null;

	Vector3 camForward = Vector3Normalize(camera.target - camera.position);
	Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, camera.up));
	Vector3 camUp = Vector3CrossProduct(camRight, camForward);

	Vector2 screenCenter = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
	float uiFrameRadius = GetUIFrameRadius();
	for (auto entity : view)
	{
		const auto &pos = view.get<Position>(entity);
		const auto &targetable = view.get<PlayerTargetable>(entity);

		if (targetable.distance > 4500)
			continue;
		Vector3 toTarget = targetable.toSelf;
		Vector3 local;
		local.x = Vector3DotProduct(toTarget, camRight);
		local.y = Vector3DotProduct(toTarget, camUp);
		local.z = Vector3DotProduct(toTarget, camForward);

		bool behind = local.z <= 0;

		Vector2 screenPos = GetWorldToScreen(pos.value, camera);

		if (behind)
		{
			screenPos.x = local.x * 0.5f + 0.5f;
			screenPos.y = local.y * 0.5f + 0.5f;
			screenPos.x *= GetScreenWidth();
			screenPos.y *= GetScreenHeight();
		}

		if (behind || screenPos.x < 0 || screenPos.x > GetScreenWidth() || screenPos.y < 0 || screenPos.y > GetScreenHeight())
		{
			Vector2 relToCenter = screenPos - screenCenter;
			Vector2 unitDir = Vector2Normalize(relToCenter);
			Vector2 arrowLoc = screenCenter + unitDir * (uiFrameRadius + 20);
			Vector2 left = { -unitDir.y, unitDir.x };

			DrawTriangle(
				arrowLoc + unitDir * 10,
				arrowLoc - left * 5,
				arrowLoc + left * 5,
				RED);
			continue;
		}

		DrawCircleLines(screenPos.x, screenPos.y, 15, RED);
		DrawCircleLines(screenPos.x, screenPos.y, 16, RED);

		if (entity == targetedEntity)
		{
			float innerRad = 17 + 5000.0f / targetable.distance;
			Color aimColor = MAROON;
			DrawRingLines(screenPos, innerRad, innerRad + 2, 90, 180, 12, aimColor);
			DrawRingLines(screenPos, innerRad, innerRad + 2, 270, 360, 12, aimColor);
			DrawLine(screenPos.x + innerRad + 2, screenPos.y, screenPos.x + innerRad + 7, screenPos.y, aimColor);
			DrawLine(screenPos.x - innerRad - 2, screenPos.y, screenPos.x - innerRad - 7, screenPos.y, aimColor);
			DrawLine(screenPos.x, screenPos.y + innerRad + 2, screenPos.x, screenPos.y + innerRad + 7, aimColor);
			DrawLine(screenPos.x, screenPos.y - innerRad - 2, screenPos.x, screenPos.y - innerRad - 7, aimColor);
		}

		char txt[32];
		snprintf(txt, sizeof(txt), "%im", targetable.distance);
		DrawText(txt, screenPos.x + 20, screenPos.y + 10, 20, MAROON);
	}
}

void Renderer::DrawHUD()
{
	// Draw the main UI frame first (background)
	DrawMainUIFrame();
	
	// Draw individual UI elements
	DrawSpeedBar();
	// DrawThrustBar();
	DrawAmmoCircle();
	DrawCrosshair();
}

Vector2 Renderer::GetUIFrameCenter() const
{
	return {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
}

float Renderer::GetUIFrameRadius() const
{
	return fminf(GetScreenWidth(), GetScreenHeight()) * 0.25f;
}

void Renderer::DrawMainUIFrame()
{
	Vector2 center = GetUIFrameCenter();
	float radius = GetUIFrameRadius();
	float startAngle = 45.0f - 90.0f;  // top right
	float endAngle = 315.0f - 90.0f;  // top left
	
	DrawRingLines(center, radius, radius + 3, startAngle, endAngle, 64, ColorAlpha(SKYBLUE, 0.8f));

	for (int i = 0; i <= 10; i++) {
		float tickAngle = startAngle + (endAngle - startAngle) * i / 10.0f;
		float tickRadius1 = radius - 5;
		float tickRadius2 = (i % 2 == 0) ? radius - 10 : radius - 15; // Longer ticks for even numbers
		
		float angleRad = tickAngle * DEG2RAD;
		Vector2 tickStart = {center.x + cosf(angleRad) * tickRadius1, center.y + sinf(angleRad) * tickRadius1};
		Vector2 tickEnd = {center.x + cosf(angleRad) * tickRadius2, center.y + sinf(angleRad) * tickRadius2};
		
		DrawLineEx(tickStart, tickEnd, 2.0f, ColorAlpha(SKYBLUE, 0.7f));
	}
}

void Renderer::DrawSpeedBar()
{
	auto playerView = context.registry.view<tag::Player, Velocity, MaxSpeed>();
	if (playerView.begin() == playerView.end()) return;
	
	auto entity = *playerView.begin();
	const auto& velocity = playerView.get<Velocity>(entity);
	const auto& maxSpeed = playerView.get<MaxSpeed>(entity);
	
	float currentSpeed = Vector3Length(velocity.value);
	float speedRatio = fminf(currentSpeed / maxSpeed.value, 1.0f);
	
	Vector2 center = GetUIFrameCenter();
	float frameRadius = GetUIFrameRadius();
	
	// Speed bar positioning - on the left side of the main arc
	float speedBarRadius = frameRadius + 13; // Outside the main frame
	float speedBarThickness = 8;
	float startAngle = 240.0f - 90.0f;  // Start slightly after main frame start
	float maxAngleRange = 50.0f;  // How much of the arc the speed bar can fill
	float currentAngleRange = maxAngleRange * speedRatio;
	
	// Background arc (full range)
	DrawRingLines(center, speedBarRadius - speedBarThickness/2, speedBarRadius + speedBarThickness/2, 
				  startAngle, startAngle + maxAngleRange, 32, ColorAlpha(DARKGRAY, 0.8f));
	DrawRingLines(center, speedBarRadius - speedBarThickness/2 + 1, speedBarRadius + speedBarThickness/2 - 1, 
				  startAngle, startAngle + maxAngleRange, 32, ColorAlpha(BLACK, 0.6f));
	
	// Interpolate between light blue and lime based on speed ratio
	// Color speedColor = ColorLerp(BLUE, SKYBLUE, speedRatio);// std::pow(speedRatio, 2));
	Color speedColor = ColorLerp(SKYBLUE, ORANGE, std::pow(speedRatio, 7));
	if (speedRatio > 0) {
		DrawRingLines(center, speedBarRadius - speedBarThickness/2 + 1, speedBarRadius + speedBarThickness/2 - 1, 
					  startAngle, startAngle + currentAngleRange, 32, speedColor);
		
	}
	// Add a glowing effect for high speeds
	if (speedRatio > 0.8f) {
		DrawRingLines(center, speedBarRadius - speedBarThickness/2, speedBarRadius + speedBarThickness/2, 
						startAngle, startAngle + currentAngleRange, 32, ColorAlpha(speedColor, 0.5f));
	}
	
	// Speed label
	float labelAngle = startAngle + maxAngleRange;
	float labelAngleRad = labelAngle * DEG2RAD;
	float labelRadius = speedBarRadius + speedBarThickness;
	Vector2 labelPos = {center.x + cosf(labelAngleRad) * labelRadius - MeasureText("SPEED", 11), 
					   center.y + sinf(labelAngleRad) * labelRadius - 8};
	DrawText("SPEED", labelPos.x, labelPos.y, 11, WHITE);
	
	// Current speed value
	char speedText[16];
	snprintf(speedText, sizeof(speedText), "%.0f", currentSpeed);
	Vector2 valuePos = {center.x + cosf(labelAngleRad) * labelRadius - MeasureText(speedText, 14), 
					    center.y + sinf(labelAngleRad) * labelRadius + 10};
	DrawText(speedText, valuePos.x, valuePos.y, 14, SKYBLUE);
	
	// Speed limit indicator
	if (maxSpeed.value > 0) {
		float safeSpeedRatio = 0.8f; // 80% of max speed is "safe"
		float safeAngle = startAngle + (maxAngleRange * safeSpeedRatio);
		float safeAngleRad = safeAngle * DEG2RAD;
		
		Vector2 safeInner = {center.x + cosf(safeAngleRad) * (speedBarRadius - speedBarThickness/2 - 2), 
							center.y + sinf(safeAngleRad) * (speedBarRadius - speedBarThickness/2 - 2)};
		Vector2 safeOuter = {center.x + cosf(safeAngleRad) * (speedBarRadius + speedBarThickness/2 + 2), 
							center.y + sinf(safeAngleRad) * (speedBarRadius + speedBarThickness/2 + 2)};
		
		DrawLineEx(safeInner, safeOuter, 3.0f, BLUE);
	}
}

void Renderer::DrawThrustBar()
{
	auto playerView = context.registry.view<tag::Player, Velocity>();
	if (playerView.begin() == playerView.end()) return;
	
	// For thrust, we'll use a simple representation based on current acceleration
	// You might want to add a Thrust component for more accurate representation
	float thrustRatio = 0.7f; // Placeholder - replace with actual thrust calculation
	
	// Position on the right side of the screen
	Vector2 barPos = {GetScreenWidth() - 70.0f, GetScreenHeight() - 150.0f};
	Vector2 barSize = {20.0f, 100.0f};
	
	// Background
	DrawRectangleRounded({barPos.x - 2, barPos.y - 2, barSize.x + 4, barSize.y + 4}, 0.2f, 8, ColorAlpha(DARKGRAY, 0.8f));
	DrawRectangleRounded({barPos.x, barPos.y, barSize.x, barSize.y}, 0.2f, 8, ColorAlpha(BLACK, 0.6f));
	
	// Thrust bar fill
	float fillHeight = barSize.y * thrustRatio;
	Color thrustColor = thrustRatio > 0.8f ? YELLOW : (thrustRatio > 0.5f ? ORANGE : BLUE);
	DrawRectangleRounded({barPos.x, barPos.y + barSize.y - fillHeight, barSize.x, fillHeight}, 0.2f, 8, thrustColor);
	
	// Label
	DrawText("THR", barPos.x - 5, barPos.y - 25, 16, WHITE);
	
	// Thrust percentage
	char thrustText[16];
	snprintf(thrustText, sizeof(thrustText), "%.0f%%", thrustRatio * 100);
	DrawText(thrustText, barPos.x - 15, barPos.y + barSize.y + 5, 14, WHITE);
}

void Renderer::DrawAmmoCircle()
{
	auto playerView = context.registry.view<tag::Player>();
	if (playerView.begin() == playerView.end()) return;
	
	auto playerEntity = *playerView.begin();
	
	// Collect all weapons with ammo for this player
	std::vector<std::pair<float, float>> weaponAmmo; // pairs of (current, max)

	if (auto ammoPtr = context.registry.try_get<Ammo>(playerEntity)) {
		weaponAmmo.push_back({ammoPtr->value, ammoPtr->maxValue});
	} else if (auto cooldownPtr = context.registry.try_get<WeaponCooldown>(playerEntity)) {
		weaponAmmo.push_back({std::min(1.0f, cooldownPtr->timeSinceLastShot / cooldownPtr->shootCooldown), 1.0});
	}

	for (auto [weaponEntity, weaponParent, ammo] : context.registry.view<WeaponParent, Ammo>().each()) {
		if (weaponParent.parent == playerEntity) {
			weaponAmmo.push_back({ammo.value, ammo.maxValue});
		}
	}

	for (auto [weaponEntity, weaponParent, cooldown] : context.registry.view<WeaponParent, WeaponCooldown>(entt::exclude<Ammo>).each()) {
		if (weaponParent.parent == playerEntity) {
			weaponAmmo.push_back({std::min(1.0f, cooldown.timeSinceLastShot / cooldown.shootCooldown), 1.0});
		}
	}

	if (weaponAmmo.empty())
		return;
	
	if (weaponAmmo.size() > 8) {
		weaponAmmo.resize(8);
	}
	
	Vector2 frameCenter = GetUIFrameCenter();
	float frameRadius = GetUIFrameRadius();
	float circleRadius = 15;

	std::vector<Vector2> positions;
	
	int weaponsPerSide = (weaponAmmo.size() + 1) / 2; // Ceiling division
	int leftSideWeapons = weaponsPerSide;
	int rightSideWeapons = weaponAmmo.size() - leftSideWeapons;
	
	// Bottom left positions
	float leftStartAngle = 240.0f - 90.0f; // Start angle for left side
	float leftGapAngle = 10.0f;  // Angle range for left side weapons
	
	for (int i = 0; i < leftSideWeapons; i++) {
		float angle = leftStartAngle + (leftGapAngle * i);
		float angleRad = angle * DEG2RAD;
		float distance = frameRadius + 60; // Distance from frame center
		
		Vector2 pos = {
			frameCenter.x + cosf(angleRad) * distance,
			frameCenter.y + sinf(angleRad) * distance
		};
		positions.push_back(pos);
	}

	std::reverse(positions.begin(), positions.end());
	
	// Bottom right positions
	float rightStartAngle = 120.0f - 90.0f; // Start angle for right side
	float rightGapAngle = 10.0f;  // Angle range for right side weapons
	
	for (int i = 0; i < rightSideWeapons; i++) {
		float angle = rightStartAngle - (rightGapAngle * i);
		float angleRad = angle * DEG2RAD;
		float distance = frameRadius + 60; // Distance from frame center
		
		Vector2 pos = {
			frameCenter.x + cosf(angleRad) * distance,
			frameCenter.y + sinf(angleRad) * distance
		};
		positions.push_back(pos);
	}
	
	// Draw each weapon's ammo circle
	for (size_t i = 0; i < weaponAmmo.size() && i < positions.size(); i++) {
		Vector2 circleCenter = positions[i];
		float currentAmmo = weaponAmmo[i].first;
		float maxAmmo = weaponAmmo[i].second;
		float ammoRatio = maxAmmo > 0 ? currentAmmo / maxAmmo : 0.0f;
		
		// Background circle
		DrawRingLines(circleCenter, circleRadius - 2, circleRadius + 2, 0, 360, 24, ColorAlpha(DARKGRAY, 0.8f));
		DrawRingLines(circleCenter, circleRadius - 1, circleRadius + 1, 0, 360, 24, ColorAlpha(BLACK, 0.6f));
		
		// Ammo arc (starts from top, goes clockwise)
		float startAngle = 0 - 90.0f; // Start from top
		float endAngle = startAngle + (360 * ammoRatio);
		Color ammoColor = ammoRatio > 0.3f ? SKYBLUE : (ammoRatio > 0.1f ? YELLOW : RED);
		
		if (ammoRatio > 0) {
			DrawRingLines(circleCenter, circleRadius - 2, circleRadius + 2, startAngle, endAngle, 32, ammoColor);
		}
		
		// Center text - show current ammo
		char ammoText[20];
		snprintf(ammoText, sizeof(ammoText), "%.0f", currentAmmo);
		int textWidth = MeasureText(ammoText, 20);
		DrawText(ammoText, circleCenter.x - textWidth/2, circleCenter.y - 7, 20, WHITE);
		
		// Weapon number indicator (small number at top of circle)
		char weaponNum[4];
		snprintf(weaponNum, sizeof(weaponNum), "%zu", i + 1);
		int numWidth = MeasureText(weaponNum, 10);
		DrawText(weaponNum, circleCenter.x - numWidth/2, circleCenter.y - circleRadius - 15, 10, LIGHTGRAY);
	}
}

void Renderer::DrawCrosshair()
{
	Vector2 center = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
	
	// Main crosshair
	float size = 15;
	float gap = 5;
	float thickness = 2;
	
	// Horizontal lines
	DrawRectangle(center.x - size - gap, center.y - thickness/2, size, thickness, WHITE);
	DrawRectangle(center.x + gap, center.y - thickness/2, size, thickness, WHITE);
	
	// Vertical lines
	DrawRectangle(center.x - thickness/2, center.y - size - gap, thickness, size, WHITE);
	DrawRectangle(center.x - thickness/2, center.y + gap, thickness, size, WHITE);
	
	// Center dot
	// DrawCircle(center.x, center.y, 2, RED);
	
	// Optional: outer crosshair ring for better visibility
	// DrawRingLines(center, 25, 27, 0, 360, 32, ColorAlpha(WHITE, 0.3f));
}
