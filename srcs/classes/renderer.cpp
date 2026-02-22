#include "renderer.hpp"
#include "components/factions.hpp"
#include "rlgl.h"
#include <iostream>
#include <algorithm>

Renderer::Renderer(Camera3D &cam, GameContext &context)
	: camera(cam), context(context)
{
	loadDefaultShader();
	loadShaderWithFallback();
	setupShaderUniforms();
}

Renderer::~Renderer()
{
	if (lightedShader.id != 0)
	{
		// UnloadShader(shader);  // this seg faults idk why
		lightedShader = {0, NULL};
	}
}

void Renderer::loadDefaultShader()
{
	defaultShader = LoadShader(NULL, NULL);
}

void Renderer::loadShaderWithFallback()
{
	lightedShader = LoadShader("shaders/sunlight.vs", "shaders/sunlight.fs");

	if (lightedShader.id == 0)
	{
		TraceLog(LOG_WARNING, "Custom shader failed to load. Using default shader.");
		lightedShader = LoadShader(NULL, NULL);
		return ;
	}

	skyboxShader = LoadShader("shaders/skybox.vs", "shaders/skybox.fs");
	if (skyboxShader.id == 0)
	{
		TraceLog(LOG_WARNING, "Custom shader failed to load. Using default shader.");
		skyboxShader = LoadShader(NULL, NULL);
		return ;
	}

	Mesh sphereMesh = GenMeshSphere(1.0f, 64, 64);
	sphereModel = LoadModelFromMesh(sphereMesh);
	sphereModel.materials[0].shader = lightedShader;

	// create a unit cone mesh (height = 1, base radius = 1) for trails
	t_model_id trailModelID = context.modelManager.loadModel("assets/Models/Trail/trail.glb");
	trailModel = context.modelManager.getModel(trailModelID);
}

void Renderer::setupShaderUniforms()
{
	lightPosLoc = GetShaderLocation(lightedShader, "lightPosition");
	lightColorLoc = GetShaderLocation(lightedShader, "lightColor");

	Vector3 lightPos = { 100000, 100000, 100000 };
	SetShaderValue(lightedShader, lightPosLoc, &lightPos, SHADER_UNIFORM_VEC3);

	Vector3 lightColor = { 1.0f, 1.0f, 1.0f };
	SetShaderValue(lightedShader, lightColorLoc, &lightColor, SHADER_UNIFORM_VEC3);
}

void Renderer::Render(float dt)
{
	currentDt = dt;
	// std::cout << "start draw\n" << std::endl;
	ClearBackground(BLACK);

	drawEntitiesWithSkyboxShader();

	BeginMode3D(camera);
	// DrawGrid(ARENA_SIZE * 2 / 10 + 1, 10);

	handleLightSource();
	drawEntitiesWithoutShader();
	drawEntitiesWithShader();
	// drawTrails();
	drawBoundaryWarning();
	drawEnergyShield();
	drawDebug();

	EndMode3D();
	// std::cout << "end draw\n" << std::endl;
}

void Renderer::drawTrails()
{
	auto trailView = context.registry.view<Position, PrevPosition, Trail>();
	for (auto entity : trailView)
	{
		const Position &p = trailView.get<Position>(entity);
		const PrevPosition &pp = trailView.get<PrevPosition>(entity);
		const Trail &t = trailView.get<Trail>(entity);
		drawTrailBetween(p.value, pp.value, t.rad, t.color);
	}
}

void Renderer::drawTrailBetween(const Vector3 &head, const Vector3 &tail, float rad, Color color)
{
	Vector3 dir = head - tail;
	float len = Vector3Length(dir);
	Vector3 mid = tail + dir * len;

	Vector3 axisOut;
	float angleOut;
	QuaternionToAxisAngle(vector3ToRotation(dir), &axisOut, &angleOut);
	DrawModelEx(trailModel, mid, axisOut, angleOut * RAD2DEG, (Vector3){rad, rad, len}, color);
}

void Renderer::handleLightSource()
{
	auto view = context.registry.view<Position, RenderBody, tag::LightSource>();

	for (auto entity : view)
	{
		const Position &pos = view.get<Position>(entity);
		const RenderBody &body = view.get<RenderBody>(entity);

		Vector3 color = {body.color.r / 255.0f, body.color.g / 255.0f, body.color.b / 255.0f};
		SetShaderValue(lightedShader, lightPosLoc, &pos.value, SHADER_UNIFORM_VEC3);
		SetShaderValue(lightedShader, lightColorLoc, &color, SHADER_UNIFORM_VEC3);
		break ;
	}
}


// if (context.registry.all_of<Rotation>(entity))
// {
// 	auto &rot = context.registry.get<Rotation>(entity);
// 	Vector3 forward = getForwardVector(rot);
// 	Vector3 end = pos.value + forward * (body.radius * 100);
// 	DrawLine3D(pos.value, end, WHITE);
// 	end = pos.value + getUpVector(rot) * (body.radius * 10);
// 	DrawLine3D(pos.value, end, GREEN);
// }

Renderer::StrechDat Renderer::getStrech(entt::entity entity) {
	StrechDat result = {1.0f, {0, 0, 0}};
	auto [pos, prevPos, strechComp] = context.registry.try_get<Position, PrevPosition, ModelStrech>(entity);
	if (!pos || !prevPos || !strechComp)
		return result;
	result.dir = Vector3Normalize(pos->value - prevPos->value);
	result.strech = std::max(1.0f, Vector3Distance(pos->value, prevPos->value) * strechComp->scale);
	return result;
}

void Renderer::drawEntityModel(const Position &pos, const RenderBody &body, StrechDat strech)
{
	Model &model = context.modelManager.getModel(body.modelID);

	Vector3 axis;
	float angle;
	QuaternionToAxisAngle(body.rotation, &axis, &angle);

	// std::cout << "Entity rotation axis: (" << axis.x << ", " << axis.y << ", " << axis.z
	//   << "), angle: " << RAD2DEG * angle << " deg" << std::endl;
	float shrink = 1; //std::max(0.01f, 1.0f / std::sqrt(strech));
	Vector3 renderScale = body.scale * Vector3{shrink, shrink, strech.strech};
	Vector3 position = pos.value + Vector3RotateByQuaternion(body.translation, body.rotation) + strech.dir * (-renderScale.z);
	DrawModelEx(model, position, axis, angle * RAD2DEG, renderScale, body.color);
}



void Renderer::drawEntitiesWithoutShader()
{
	auto view = context.registry.view<Position, RenderBody>(entt::exclude<tag::Shaded, tag::SkyBox>);

	for (auto entity : view)
	{
		const Position &pos = view.get<Position>(entity);
		const RenderBody &body = view.get<RenderBody>(entity);
		Model &model = context.modelManager.getModel(body.modelID);
		for (int i = 0; i < model.materialCount; i++) {
			model.materials[i].shader = defaultShader;
		}
		drawEntityModel(pos, body, getStrech(entity));
	}
}

void Renderer::drawEntitiesWithShader()
{
	// BeginShaderMode(lightedShader);

	auto view = context.registry.view<Position, RenderBody, tag::Shaded>();
	for (auto entity : view)
	{
		const Position &pos = view.get<Position>(entity);
		const RenderBody &body = view.get<RenderBody>(entity);
		Model &model = context.modelManager.getModel(body.modelID);
		for (int i = 0; i < model.materialCount; i++) {
			model.materials[i].shader = lightedShader;
		}
		// SetShaderValueTexture(shader, GetShaderLocation(shader, "texture0"), model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
		drawEntityModel(pos, body, getStrech(entity));
	}

	// EndShaderMode();
}

void Renderer::drawEntitiesWithSkyboxShader()
{
	// BeginShaderMode(skyboxShader);
	Camera3D centerCam = camera;
	centerCam.position = {0, 0, 0};
	centerCam.target = Vector3Normalize(camera.target - camera.position) * 0.01f;

	BeginMode3D(centerCam);
	rlDisableDepthMask();
	rlDisableBackfaceCulling();

	auto view = context.registry.view<Position, RenderBody, tag::SkyBox>();
	for (auto entity : view)
	{
		const Position &pos = view.get<Position>(entity);
		const RenderBody &body = view.get<RenderBody>(entity);
		Model &model = context.modelManager.getModel(body.modelID);
		for (int i = 0; i < model.materialCount; i++) {
			model.materials[i].shader = skyboxShader;
		}
		// std::cout << "drawing " << model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture.height << std::endl;
		drawEntityModel(pos, body);
	}

	rlEnableBackfaceCulling();
	rlEnableDepthMask();
	EndMode3D();
	// EndShaderMode();
}

void Renderer::drawEnergyShield()
{
	auto view = context.registry.view<Position, RenderBody, EnergyShield>();
	t_model_id model = context.modelManager.loadModel("assets/Models/shield/spherical_hex_force_field.glb", 0.01f);

	for (auto [entity, pos, body, shield] : view.each())
	{
		if (shield.activeTimer <= 0.0f || shield.hp < 10)
			continue;
		context.modelManager.getModel(body.modelID).materials[0].shader = defaultShader;

		Color color = ColorAlpha(SKYBLUE, (0.1 + 0.5 * shield.hp / shield.maxHp) * (shield.activeTimer / shield.activeDuration));
		float scale = std::max(body.scale.x, std::max(body.scale.y, body.scale.z)) * 4;
		DrawModel(context.modelManager.getModel(model), pos.value, scale, color);
	}
}

void Renderer::drawBoundaryWarning()
{
	if (!context.registry.valid(context.currentPlayer))
		return;
	
	auto posPtr = context.registry.try_get<Position>(context.currentPlayer);
	if (!posPtr)
		return;
	
	Vector3 playerPos = posPtr->value;
	
	const float softBoundaryStart = context.config.ARENA_SIZE * 0.5f;
	const float hardBoundary = context.config.ARENA_SIZE;
	const float warningZone = hardBoundary - softBoundaryStart;
	
	Vector3 axes[3] = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
	float positions[3] = {playerPos.x, playerPos.y, playerPos.z};
	
	for (int axis = 0; axis < 3; axis++) {
		float currentPos = positions[axis];
		float absCurrentPos = std::abs(currentPos);
		
		if (absCurrentPos < softBoundaryStart)
			continue;

		float excess = absCurrentPos - softBoundaryStart;
		float intensity = std::min(1.0f, excess / warningZone);
		
		if (intensity <= 0.0f)
			continue;
		
		Vector3 toBoundaryUnit = axes[axis] * (currentPos > 0 ? 1.0f : -1.0f);
		
		Vector3 planeCenter = playerPos * (Vector3Ones - toBoundaryUnit * toBoundaryUnit) + toBoundaryUnit * hardBoundary;
		Vector3 right, up;
		
		// Generate perpendicular vectors for the grid plane
		if (std::abs(toBoundaryUnit.y) < 0.9f) {
			right = Vector3Normalize(Vector3CrossProduct(toBoundaryUnit, {0, 1, 0}));
		} else {
			right = Vector3Normalize(Vector3CrossProduct(toBoundaryUnit, {1, 0, 0}));
		}
		up = Vector3Normalize(Vector3CrossProduct(right, toBoundaryUnit));
		
		// Grid parameters
		const float gridSize = 200.0f;  // Total grid size
		const float gridTileSize = 20.0f;
		const int linesPerSide = (int)(gridSize / gridTileSize) + 1;
		const float halfGrid = gridSize * 0.5f;
		const Vector3 boundVec = {context.config.ARENA_SIZE, context.config.ARENA_SIZE, context.config.ARENA_SIZE};

		// Calculate grid offset and snap to grid tile size
		Vector3 playerProjection = playerPos * (Vector3Ones - toBoundaryUnit * toBoundaryUnit);
		float rightOffset = fmodf(Vector3DotProduct(playerProjection, right), gridTileSize);
		float upOffset = fmodf(Vector3DotProduct(playerProjection, up), gridTileSize);
		
		float alpha = intensity * 0.3f;
		Color warningColor = ColorAlpha(WHITE, alpha);
		
		// Draw horizontal grid lines
		for (int i = 0; i < linesPerSide; i++) {
			float linePos = (i * gridTileSize) - halfGrid - upOffset;
			
			Vector3 lineStart = planeCenter + right * (-halfGrid - rightOffset) + up * linePos;
			Vector3 lineEnd = planeCenter + right * (halfGrid - rightOffset) + up * linePos;
			
			lineStart = Vector3Clamp(lineStart, boundVec * -1, boundVec);
			lineEnd = Vector3Clamp(lineEnd, boundVec * -1, boundVec);
			DrawLine3D(lineStart, lineEnd, warningColor);
		}
		
		// Draw vertical grid lines
		for (int i = 0; i < linesPerSide; i++) {
			float linePos = (i * gridTileSize) - halfGrid - rightOffset;
			
			Vector3 lineStart = planeCenter + right * linePos + up * (-halfGrid - upOffset);
			Vector3 lineEnd = planeCenter + right * linePos + up * (halfGrid - upOffset);
			
			lineStart = Vector3Clamp(lineStart, boundVec * -1, boundVec);
			lineEnd = Vector3Clamp(lineEnd, boundVec * -1, boundVec);
			DrawLine3D(lineStart, lineEnd, warningColor);
		}
	}
}

void Renderer::drawDebug()
{
	if (!context.config.debug.showTarget)
		return;

	auto view = context.registry.view<Position, TargetRotation>();
	for (auto [entity, pos, tRot] : view.each())
	{
		Vector3 start = pos.value;
		Vector3 forward = getForwardVector(tRot.value);

		Vector3 end = start + forward * 15.0f;

		DrawCylinderEx(start, end, 0.2f, 0.2f, 8, RED);
		DrawCylinderEx(end, end + forward * 3.0f, 0.6f, 0.0f, 8, RED);
	}
}