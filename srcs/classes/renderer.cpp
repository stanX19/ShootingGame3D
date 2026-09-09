#include "renderer.hpp"
#include "components/factions.hpp"
#include "rlgl.h"
#include <iostream>
#include <algorithm>

Renderer::Renderer(Camera3D &cam, GameContext &context)
	: m_camera(cam), m_context(context)
{
	loadDefaultShader();
	loadShaderWithFallback();
	setupShaderUniforms();
}

Renderer::~Renderer()
{
	if (m_lightedShader.id > 0 && m_lightedShader.id != rlGetShaderIdDefault())
	{
		UnloadShader(m_lightedShader);
		m_lightedShader = {0, nullptr};
	}

	if (m_skyboxShader.id > 0 && m_skyboxShader.id != rlGetShaderIdDefault())
	{
		UnloadShader(m_skyboxShader);
		m_skyboxShader = {0, nullptr};
	}

	if (m_defaultShader.id > 0 && m_defaultShader.id != rlGetShaderIdDefault())
	{
		UnloadShader(m_defaultShader);
		m_defaultShader = {0, nullptr};
	}
}

void Renderer::loadDefaultShader()
{
	m_defaultShader = LoadShader(NULL, NULL);
}

void Renderer::loadShaderWithFallback()
{
	m_lightedShader = LoadShader("shaders/sunlight.vs", "shaders/sunlight.fs");
	if (m_lightedShader.id == 0)
	{
		TraceLog(LOG_WARNING, "Custom shader failed to load. Using default shader.");
		m_lightedShader = LoadShader(NULL, NULL);
	}

	m_skyboxShader = LoadShader("shaders/skybox.vs", "shaders/skybox.fs");
	if (m_skyboxShader.id == 0)
	{
		TraceLog(LOG_WARNING, "Custom shader failed to load. Using default shader.");
		m_skyboxShader = LoadShader(NULL, NULL);
	}

	// create a unit cone mesh (height = 1, base radius = 1) for trails
	const t_model_id trailModelID = m_context.modelManager.loadModel("assets/Models/Trail/trail.glb");
	m_trailModel = m_context.modelManager.getModel(trailModelID);
}

void Renderer::setupShaderUniforms()
{
	m_lightPosLoc = GetShaderLocation(m_lightedShader, "lightPosition");
	m_lightColorLoc = GetShaderLocation(m_lightedShader, "lightColor");
	m_normalMapAvailableLoc = GetShaderLocation(m_lightedShader, "normalMapAvailable");

	Vector3 lightPos = { 100000, 100000, 100000 };
	SetShaderValue(m_lightedShader, m_lightPosLoc, &lightPos, SHADER_UNIFORM_VEC3);

	Vector3 lightColor = { 1.0f, 1.0f, 1.0f };
	SetShaderValue(m_lightedShader, m_lightColorLoc, &lightColor, SHADER_UNIFORM_VEC3);
}

void Renderer::updateFrustum()
{
	Matrix viewMat = rlGetMatrixModelview();
	Matrix projMat = rlGetMatrixProjection();
	Matrix viewProjMat = MatrixMultiply(viewMat, projMat);
	m_currentFrustum = Frustum::fromViewProjection(viewProjMat);
}

bool Renderer::isEntityVisible(entt::entity entity, const Position &pos, const RenderBody &body, StrechDat &strech) const
{
	strech = getStrech(entity);
	const float baseRadius = m_context.modelManager.getModelRadius(body.modelID);
	const float maxScale = std::max({body.scale.x, body.scale.y, body.scale.z, 0.01f});
	const float translationLen = Vector3Length(body.translation);
	const float effectiveRadius = baseRadius * maxScale + translationLen + (strech.strech > 1.0f ? strech.strech * maxScale : 0.0f);
	return m_currentFrustum.isSphereInside(pos.value, effectiveRadius);
}

void Renderer::render(float dt)
{
	m_currentDt = dt;
	// std::cout << "start draw\n" << std::endl;
	ClearBackground(BLACK);

	drawEntitiesWithSkyboxShader();

	BeginMode3D(m_camera);
	updateFrustum();
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
	auto trailView = m_context.registry.view<Position, PrevPosition, Trail>();
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
	const Vector3 dir = head - tail;
	const float len = Vector3Length(dir);
	const Vector3 mid = tail + dir * len;

	Vector3 axisOut;
	float angleOut;
	QuaternionToAxisAngle(vector3ToRotation(dir), &axisOut, &angleOut);
	DrawModelEx(m_trailModel, mid, axisOut, angleOut * RAD2DEG, (Vector3){rad, rad, len}, color);
}

void Renderer::handleLightSource()
{
	auto view = m_context.registry.view<Position, RenderBody, tag::LightSource>();

	for (auto entity : view)
	{
		const Position &pos = view.get<Position>(entity);
		const RenderBody &body = view.get<RenderBody>(entity);

		Vector3 color = {body.color.r / 255.0f, body.color.g / 255.0f, body.color.b / 255.0f};
		SetShaderValue(m_lightedShader, m_lightPosLoc, &pos.value, SHADER_UNIFORM_VEC3);
		SetShaderValue(m_lightedShader, m_lightColorLoc, &color, SHADER_UNIFORM_VEC3);
		break ;
	}
}

Renderer::StrechDat Renderer::getStrech(entt::entity entity) const {
	StrechDat result = {1.0f, {0, 0, 0}};
	auto [pos, prevPos, strechComp] = m_context.registry.try_get<Position, PrevPosition, ModelStrech>(entity);
	if (!pos || !prevPos || !strechComp)
		return result;
	result.dir = Vector3Normalize(pos->value - prevPos->value);
	result.strech = std::max(1.0f, Vector3Distance(pos->value, prevPos->value) * strechComp->scale);
	return result;
}

void Renderer::drawEntityModel(const Position &pos, const RenderBody &body, StrechDat strech)
{
	Model &model = m_context.modelManager.getModel(body.modelID);

	Vector3 axis;
	float angle;
	QuaternionToAxisAngle(body.rotation, &axis, &angle);

	const float shrink = 1.0f;
	const Vector3 renderScale = body.scale * Vector3{shrink, shrink, strech.strech};
	const Vector3 position = pos.value + Vector3RotateByQuaternion(body.translation, body.rotation) + strech.dir * (-renderScale.z);
	DrawModelEx(model, position, axis, angle * RAD2DEG, renderScale, body.color);
}

void Renderer::drawEntitiesWithoutShader()
{
	auto view = m_context.registry.view<Position, RenderBody>(entt::exclude<tag::Shaded, tag::SkyBox>);

	for (auto entity : view)
	{
		const Position &pos = view.get<Position>(entity);
		const RenderBody &body = view.get<RenderBody>(entity);
		StrechDat strech;
		if (!isEntityVisible(entity, pos, body, strech))
			continue;

		Model &model = m_context.modelManager.getModel(body.modelID);
		for (int i = 0; i < model.materialCount; i++) {
			model.materials[i].shader = m_defaultShader;
		}
		drawEntityModel(pos, body, strech);
	}
}

void Renderer::drawEntitiesWithShader()
{
	auto view = m_context.registry.view<Position, RenderBody, tag::Shaded>();
	for (auto entity : view)
	{
		const Position &pos = view.get<Position>(entity);
		const RenderBody &body = view.get<RenderBody>(entity);
		StrechDat strech;
		if (!isEntityVisible(entity, pos, body, strech))
			continue;

		Model &model = m_context.modelManager.getModel(body.modelID);
		bool hasNormalMap = false;
		for (int i = 0; i < model.materialCount; i++) {
			model.materials[i].shader = m_lightedShader;
			if (model.materials[i].maps != nullptr && model.materials[i].maps[MATERIAL_MAP_NORMAL].texture.id > 0)
				hasNormalMap = true;
		}
		const int normalMapAvailable = hasNormalMap ? 1 : 0;
		SetShaderValue(m_lightedShader, m_normalMapAvailableLoc, &normalMapAvailable, SHADER_UNIFORM_INT);
		drawEntityModel(pos, body, strech);
	}
}

void Renderer::drawEntitiesWithSkyboxShader()
{
	Camera3D centerCam = m_camera;
	centerCam.position = {0, 0, 0};
	centerCam.target = Vector3Normalize(m_camera.target - m_camera.position) * 0.01f;

	BeginMode3D(centerCam);
	rlDisableDepthMask();
	rlDisableBackfaceCulling();

	auto view = m_context.registry.view<Position, RenderBody, tag::SkyBox>();
	for (auto entity : view)
	{
		const Position &pos = view.get<Position>(entity);
		const RenderBody &body = view.get<RenderBody>(entity);
		Model &model = m_context.modelManager.getModel(body.modelID);
		for (int i = 0; i < model.materialCount; i++) {
			model.materials[i].shader = m_skyboxShader;
		}
		drawEntityModel(pos, body);
	}

	rlEnableBackfaceCulling();
	rlEnableDepthMask();
	EndMode3D();
}

void Renderer::drawEnergyShield()
{
	auto view = m_context.registry.view<Position, RenderBody, EnergyShield>();
	const t_model_id model = m_context.modelManager.loadModel("assets/Models/shield/spherical_hex_force_field.glb", 0.01f);

	for (auto [entity, pos, body, shield] : view.each())
	{
		if (shield.activeTimer <= 0.0f || shield.hp < 10)
			continue;
		const float scale = std::max(body.scale.x, std::max(body.scale.y, body.scale.z)) * 4;
		if (!m_currentFrustum.isSphereInside(pos.value, scale))
			continue;

		m_context.modelManager.getModel(body.modelID).materials[0].shader = m_defaultShader;

		const Color color = ColorAlpha(SKYBLUE, (0.1 + 0.5 * shield.hp / shield.maxHp) * (shield.activeTimer / shield.activeDuration));
		DrawModel(m_context.modelManager.getModel(model), pos.value, scale, color);
	}
}

void Renderer::drawBoundaryWarning()
{
	if (!m_context.registry.valid(m_context.currentPlayer))
		return;
	
	const auto posPtr = m_context.registry.try_get<Position>(m_context.currentPlayer);
	if (!posPtr)
		return;
	
	const Vector3 playerPos = posPtr->value;
	
	const float softBoundaryStart = m_context.config.ARENA_SIZE * 0.5f;
	const float hardBoundary = m_context.config.ARENA_SIZE;
	const float warningZone = hardBoundary - softBoundaryStart;
	
	const Vector3 axes[3] = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
	const float positions[3] = {playerPos.x, playerPos.y, playerPos.z};
	
	for (int axis = 0; axis < 3; axis++) {
		const float currentPos = positions[axis];
		const float absCurrentPos = std::abs(currentPos);
		
		if (absCurrentPos < softBoundaryStart)
			continue;

		const float excess = absCurrentPos - softBoundaryStart;
		const float intensity = std::min(1.0f, excess / warningZone);
		
		if (intensity <= 0.0f)
			continue;
		
		const Vector3 toBoundaryUnit = axes[axis] * (currentPos > 0 ? 1.0f : -1.0f);
		
		const Vector3 planeCenter = playerPos * (Vector3Ones - toBoundaryUnit * toBoundaryUnit) + toBoundaryUnit * hardBoundary;
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
		const Vector3 boundVec = {m_context.config.ARENA_SIZE, m_context.config.ARENA_SIZE, m_context.config.ARENA_SIZE};

		// Calculate grid offset and snap to grid tile size
		const Vector3 playerProjection = playerPos * (Vector3Ones - toBoundaryUnit * toBoundaryUnit);
		const float rightOffset = fmodf(Vector3DotProduct(playerProjection, right), gridTileSize);
		const float upOffset = fmodf(Vector3DotProduct(playerProjection, up), gridTileSize);
		
		const float alpha = intensity * 0.3f;
		const Color warningColor = ColorAlpha(WHITE, alpha);
		
		// Draw horizontal grid lines
		for (int i = 0; i < linesPerSide; i++) {
			const float linePos = (i * gridTileSize) - halfGrid - upOffset;
			
			Vector3 lineStart = planeCenter + right * (-halfGrid - rightOffset) + up * linePos;
			Vector3 lineEnd = planeCenter + right * (halfGrid - rightOffset) + up * linePos;
			
			lineStart = Vector3Clamp(lineStart, boundVec * -1, boundVec);
			lineEnd = Vector3Clamp(lineEnd, boundVec * -1, boundVec);
			DrawLine3D(lineStart, lineEnd, warningColor);
		}
		
		// Draw vertical grid lines
		for (int i = 0; i < linesPerSide; i++) {
			const float linePos = (i * gridTileSize) - halfGrid - rightOffset;
			
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
	if (!m_context.config.debug.showTarget)
		return;

	auto view = m_context.registry.view<Position, TargetRotation>();
	for (auto [entity, pos, tRot] : view.each())
	{
		const Vector3 start = pos.value;
		const Vector3 forward = getForwardVector(tRot.value);

		const Vector3 end = start + forward * 15.0f;

		DrawCylinderEx(start, end, 0.2f, 0.2f, 8, RED);
		DrawCylinderEx(end, end + forward * 3.0f, 0.6f, 0.0f, 8, RED);
	}
}
