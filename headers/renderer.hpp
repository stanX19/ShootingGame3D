#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "includes.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "utils.hpp"

class Renderer {
public:
    Renderer(Camera3D& camera, entt::registry& registry);
    ~Renderer();

    void Render();

private:
    Camera3D& camera;
    entt::registry& registry;
    Shader shader;
	
	Model sphereModel;
	int lightPosLoc;
	int lightColorLoc;
	int ambientStrengthLoc;

    void LoadShaderWithFallback();
    void SetupShaderUniforms();
    void DrawEntitiesWithShader();
    void DrawEntitiesWithoutShader();
    void HandleLightSource();
    void DrawHealthBars();
    void DrawTargetable();
    void DrawTexts();
};

#endif