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

	struct BatchKey {
		t_model_id modelId;
		Color color;
		bool shaded;

		bool operator<(const BatchKey &other) const {
			if (modelId != other.modelId) return modelId < other.modelId;
			if (color.r != other.color.r) return color.r < other.color.r;
			if (color.g != other.color.g) return color.g < other.color.g;
			if (color.b != other.color.b) return color.b < other.color.b;
			if (color.a != other.color.a) return color.a < other.color.a;
			return shaded < other.shaded;
		}
	};

	std::map<BatchKey, std::vector<Matrix>> m_instancedBatches;
	std::vector<Matrix> m_tempTransformBuffer;

	void loadDefaultShader();
	void loadShaderWithFallback();
	void setupShaderUniforms();
	void updateFrustum();
	StrechDat getStrech(entt::entity entity) const;
	bool isEntityVisible(entt::entity entity, const physics::Position &pos, const render::RenderBody &body, StrechDat &strech) const;
	void drawEntityModel(const physics::Position &pos, const render::RenderBody &body, StrechDat strech = {1.0f, {0,0,0}});
	void drawTrails();
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
