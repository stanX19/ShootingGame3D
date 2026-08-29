#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "includes.hpp"
#include "components.hpp"
#include "utils.hpp"
#include "game_context.hpp"
#include "frustum.hpp"


class Renderer {
	friend class BenchmarkRenderer;
public:
	Renderer(Camera3D& camera, GameContext &context);
	~Renderer();
	Renderer(const Renderer &) = delete;
	Renderer &operator=(const Renderer &) = delete;

	void Render(float dt);

private:
	Camera3D& camera;
	GameContext &context;
	float currentDt = 0.0f;
	Shader lightedShader{};
	Shader skyboxShader{};
	Shader defaultShader{};
	
	Model trailModel{};
	int lightPosLoc = 0;
	int lightColorLoc = 0;
	int ambientStrengthLoc = 0;
	int normalMapAvailableLoc = 0;
	Frustum currentFrustum;

	struct StrechDat {
		float strech;
		Vector3 dir;
	};

	void loadDefaultShader();
	void loadShaderWithFallback();
	void setupShaderUniforms();
	void updateFrustum();
	StrechDat getStrech(entt::entity entity);
	bool isEntityVisible(entt::entity entity, const Position &pos, const RenderBody &body, StrechDat &strech);
	void drawEntityModel(const Position &pos, const RenderBody &body, StrechDat strech = {1.0f, {0,0,0}});
	void drawTrails();
	void drawTrailBetween(const Vector3 &head, const Vector3 &tail, float rad, Color color);
	void drawEntitiesWithShader();
	void drawEntitiesWithSkyboxShader();
	void drawEntitiesWithoutShader();
	void drawBoundaryWarning();
	void drawEnergyShield();
	void handleLightSource();
	void drawDebug();
};

#endif
