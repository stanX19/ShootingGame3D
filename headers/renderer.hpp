#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "includes.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include "game_context.hpp"

class Renderer {
public:
	Renderer(Camera3D& camera, GameContext &context);
	~Renderer();

	void Render();

private:
	Camera3D& camera;
	GameContext &context;
	Shader lightedShader;
	Shader skyboxShader;
	Shader defaultShader;
	
	Model sphereModel;
	int lightPosLoc;
	int lightColorLoc;
	int ambientStrengthLoc;

	void loadDefaultShader();
	void loadShaderWithFallback();
	void setupShaderUniforms();
	void drawEntityModel(const Position &pos, const RenderBody &body, float strech = 1.0f);
	void drawEntitiesWithShader();
	void drawEntitiesWithSkyboxShader();
	void drawEntitiesWithoutShader();
	void drawBoundaryWarning();
	void drawEnergyShield();
	void handleLightSource();
};

#endif