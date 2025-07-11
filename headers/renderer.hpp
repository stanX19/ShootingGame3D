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
	Shader shader;
	Shader defaultShader;
	
	Model sphereModel;
	int lightPosLoc;
	int lightColorLoc;
	int ambientStrengthLoc;

	void LoadDefaultShader();
	void LoadShaderWithFallback();
	void SetupShaderUniforms();
	void DrawEntityModel(const Position &pos, const RenderBody &body);
	void DrawEntitiesWithShader();
	void DrawEntitiesWithoutShader();
	void DrawBoundaryWarning();
	void HandleLightSource();
	
	void DrawHUD();
	void DrawHealthBars();
	void DrawTargetable();
	void DrawTexts();
	void DrawSpeedBar();
	void DrawThrustBar();
	void DrawAmmoCircle();
	void DrawCrosshair();
	void DrawMainUIFrame();
	void DrawCursorArrow();
	void DrawCollisionWarning();
	
	Vector2 GetUIFrameCenter() const;
	float GetUIFrameRadius() const;
};

#endif