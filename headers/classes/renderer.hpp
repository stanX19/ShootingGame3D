#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "includes.hpp"
#include "components/physics.hpp"
#include "components/render.hpp"
#include "components/effect.hpp"
#include "components/combat.hpp"
#include "components/spaceship.hpp"
#include "utils.hpp"
#include "game_context.hpp"
#include "frustum.hpp"


class Renderer {
	friend class BenchmarkRenderer;
public:
	explicit Renderer(GameContext &context);
	Renderer(Camera3D&, GameContext &context) : Renderer(context) {}
	~Renderer();
	Renderer(const Renderer &) = delete;
	Renderer &operator=(const Renderer &) = delete;

	void render(float dt, const Camera3D &camera);
	void Render(float dt, const Camera3D &camera) { render(dt, camera); }

private:
	Camera3D m_camera{};
	GameContext &m_context;
	float m_currentDt = 0.0f;
	Shader m_lightedShader{};
	Shader m_skyboxShader{};
	Shader m_defaultShader{};
	Shader m_instancedShader{};
	Shader m_instancedLightedShader{};
	
	Model m_trailModel{};
	int m_lightPosLoc = 0;
	int m_lightColorLoc = 0;
	int m_ambientStrengthLoc = 0;
	int m_normalMapAvailableLoc = 0;
	int m_instancedNormalMapAvailableLoc = 0;
	Frustum m_currentFrustum;

	struct StrechDat {
		float strech;
		Vector3 dir;
	};

	struct ModelInstanceGroup {
		uint64_t key = 0;
		Color color{255, 255, 255, 255};
		bool shaded = false;
		std::vector<Matrix> transforms;
	};

	struct ModelBatch {
		std::vector<ModelInstanceGroup> groups;
	};

	std::vector<ModelBatch> m_modelBatches;
	std::vector<Matrix> m_tempTransformBuffer;

	void loadDefaultShader();
	void loadShaderWithFallback();
	void setupShaderUniforms();
	void updateFrustum();
	StrechDat getStrech(entt::entity entity) const;
	bool isEntityVisible(entt::entity entity, const physics::Position &pos, const render::RenderBody &body, StrechDat &strech) const;
	void drawEntityModel(const physics::Position &pos, const render::RenderBody &body, StrechDat strech = {1.0f, {0,0,0}});
	void drawTrails();
	void drawSimpleRibbon(const effect::HasSimpleTrail &trail);
	void drawMultiRibbon(const effect::HasMultiTrail::Emitter &emitter, const effect::HasMultiTrail &trail);
	void drawTrailBetween(const Vector3 &head, const Vector3 &tail, float rad, Color color);
	void drawEntitiesBatched();
	void drawEntitiesWithShader();
	void drawEntitiesWithSkyboxShader();
	void drawEntitiesWithoutShader();
	void drawBoundaryWarning();
	void drawEnergyShield();
	void handleLightSource();
	void drawDebug();
};

#endif
