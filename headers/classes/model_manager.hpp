#ifndef MODEL_MANAGER_HPP
#define MODEL_MANAGER_HPP

#include "includes.hpp"
#include "game_config.hpp"
#include "op_overloads.hpp"

#include <optional>

using t_model_id = size_t;

class ModelManager {
public:
	ModelManager();
	~ModelManager();
	ModelManager(const ModelManager &) = delete;
	ModelManager &operator=(const ModelManager &) = delete;

	t_model_id loadModel(const std::string& filePath);
	t_model_id loadModel(const std::string &filePath, float scale);
	t_model_id loadModel(const std::string &filePath, const Vector3 &scale);
	t_model_id loadModel(const std::string &filePath, const Vector3 &scale, const Vector3 &rotation, const Vector3 &displacement);
	t_model_id loadModel(const std::string &filePath, const Matrix &transform);

	t_model_id loadModel(const GameConfig& config, const std::string& configPath);

	t_model_id createCube(float width = 2.0f, float height = 2.0f, float length = 2.0f);
	t_model_id createSphere(int rings = 16, int slices = 16, float radius = 1.0f);
	t_model_id createCylinder(int slices = 16, float radius = 1.0f, float height = 2.0f);
	t_model_id createPlane(float width = 2.0f, float length = 2.0f, int resX = 4, int resZ = 4);
	t_model_id createTriangle(const Vector3 &p1, const Vector3 &p2, const Vector3 &p3);
	t_model_id createTriangle(float rad = 1.0f, float angle1 = 0.0f, float angle2 = 2.0943951f, float angle3 = 4.1887902f);

	Model& getModel(t_model_id id);
	const Model& getModel(t_model_id id) const;
	std::optional<std::string> getModelPath(t_model_id id) const;

	void unloadAll();
	float getModelRadius(t_model_id id) const;

	bool isValid(t_model_id id) const;
private:
	std::vector<Model> m_models;
	std::vector<float> m_modelRadii;
	std::vector<std::optional<std::string>> m_modelPaths;
	std::map<std::pair<std::string, Matrix>, t_model_id> m_loadedFromFile; // filepath -> id
	std::map<std::string, t_model_id> m_proceduralCache;

	template <typename... Args>
	std::string generateCacheKey(const std::string &keyBase, Args&&... args) const;
	template <typename Func, typename... Args>
	t_model_id createAndAddModel(const std::string& keyBase, Func modelGenerator, Args&&... args);
};

#endif  // MODEL_MANAGER_HPP
