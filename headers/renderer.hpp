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
	
	int modelLoc;
	int mvpLoc;
    int lightDirLoc;
    int lightColorLoc;
    int ambientColorLoc;
    int objectColorLoc;

    void LoadShaderWithFallback();
    void SetupShaderUniforms();
    void DrawEntitiesWithShader();
    void DrawHealthBars();
    void DrawTargetable();
};

#endif