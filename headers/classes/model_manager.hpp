#ifndef MODEL_MANAGER_HPP
#define MODEL_MANAGER_HPP

#include "includes.hpp"
#include "game_config.hpp"
#include "op_overloads.hpp"

using t_model_id = size_t;

class ModelManager {
public:
	ModelManager();
	~ModelManager();

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

	Model& getModel(t_model_id id);
	const Model& getModel(t_model_id id) const;

	void unloadAll();

	bool isValid(t_model_id id) const;
private:
	std::vector<Model> models;
	std::map<std::pair<std::string, Matrix>, t_model_id> loadedFromFile; // filepath -> id
	std::map<std::string, t_model_id> proceduralCache;

	template <typename... Args>
	std::string generateCacheKey(const std::string &keyBase, Args&&... args) const;
	template <typename Func, typename... Args>
	t_model_id createAndAddModel(const std::string& keyBase, Func modelGenerator, Args&&... args);
};

#endif  // MODEL_MANAGER_HPP