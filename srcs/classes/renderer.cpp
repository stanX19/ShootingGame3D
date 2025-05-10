#include "renderer.hpp"
#include <iostream>

Renderer::Renderer(Camera3D &cam, entt::registry &reg)
	: camera(cam), registry(reg)
{
	LoadShaderWithFallback();
	SetupShaderUniforms();
}

Renderer::~Renderer()
{
	if (shader.id != 0)
	{
		UnloadShader(shader);
		shader = {0, NULL};
	}
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
	DrawFPS(10, 10);

	DrawTexts();

	EndDrawing();
}

void Renderer::DrawTexts() {
	auto playerView = registry.view<tag::Player, HP>();

	if (playerView.begin() != playerView.end())
	{
		int totalEntities = 0;
		auto hittableView = registry.view<CollisionBody, Position, HP>();
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
	auto view = registry.view<Position, RenderBody, tag::LightSource>();

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

void Renderer::DrawEntitiesWithoutShader()
{
	auto view = registry.view<Position, RenderBody>(entt::exclude<tag::Shaded>);

	for (auto entity : view)
	{
        const Position &pos = view.get<Position>(entity);
        const RenderBody &body = view.get<RenderBody>(entity);
        DrawSphere(pos.value, body.radius, body.color);
    }
}

// if (registry.all_of<Rotation>(entity))
// {
// 	auto &rot = registry.get<Rotation>(entity);
// 	Vector3 forward = GetForwardVector(rot);
// 	Vector3 end = pos.value + forward * (body.radius * 100);
// 	DrawLine3D(pos.value, end, WHITE);
// 	end = pos.value + GetUpVector(rot) * (body.radius * 10);
// 	DrawLine3D(pos.value, end, GREEN);
// }

void Renderer::DrawEntitiesWithShader()
{
	BeginShaderMode(shader);

	auto view = registry.view<Position, RenderBody, tag::Shaded>();
	for (auto entity : view)
	{
        const Position &pos = view.get<Position>(entity);
        const RenderBody &body = view.get<RenderBody>(entity);

        Vector3 scale = { body.radius, body.radius, body.radius };
        DrawModelEx(sphereModel, pos.value, {0, 1, 0}, 0.0f, scale, body.color);
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
	auto view = registry.view<Position, CollisionBody, HP>();
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
	auto view = registry.view<Position, PlayerTargetable>();
	auto playerView = registry.view<tag::Player, AimTarget>();

	entt::entity targetedEntity = playerView.begin() != playerView.end() ? playerView.get<AimTarget>(*playerView.begin()).entity : entt::null;

	Vector3 camForward = Vector3Normalize(camera.target - camera.position);
	Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, camera.up));
	Vector3 camUp = Vector3CrossProduct(camRight, camForward);

	Vector2 screenCenter = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };

	for (auto entity : view)
	{
		const auto &pos = view.get<Position>(entity);
		const auto &targetable = view.get<PlayerTargetable>(entity);

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
			Vector2 arrowLoc = screenCenter + unitDir * 250;
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
