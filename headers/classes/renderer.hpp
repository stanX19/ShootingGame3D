#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "includes.hpp"
#include "components.hpp"
#include "utils.hpp"
#include "game_context.hpp"


class Renderer {
public:
	Renderer(Camera3D& camera, GameContext &context);
	~Renderer();

	void Render(float dt);

private:
	Camera3D& camera;
	GameContext &context;
	float currentDt;
	Shader lightedShader;
	Shader skyboxShader;
	Shader defaultShader;
	
	Model sphereModel;
	Model trailModel;
	int lightPosLoc;
	int lightColorLoc;
	int ambientStrengthLoc;
	int normalMapAvailableLoc;

	struct StrechDat {
		float strech;
		Vector3 dir;
	};

	void loadDefaultShader();
	void loadShaderWithFallback();
	void setupShaderUniforms();
	StrechDat getStrech(entt::entity entity);
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
