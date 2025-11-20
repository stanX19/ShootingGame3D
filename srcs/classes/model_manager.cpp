#include <sstream>
#include <string>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <iostream>
#include "model_manager.hpp"
#include "utils.hpp"

ModelManager::ModelManager() {}

ModelManager::~ModelManager()
{
	unloadAll();
}

t_model_id ModelManager::loadModel(const std::string &filePath, const Matrix &transform)
{
	auto key = std::make_pair(filePath, transform);
	auto it = loadedFromFile.find(key);
	if (it != loadedFromFile.end())
	{
		return it->second;
	}

	std::filesystem::path originalPath = std::filesystem::current_path();
	std::filesystem::path modelPath = std::filesystem::absolute(filePath);
	std::filesystem::path modelDir = modelPath.parent_path();
	std::filesystem::path modelFile = modelPath.filename();

	if (!std::filesystem::exists(modelDir))
	{
		throw std::runtime_error("Model directory does not exist: " + modelDir.string());
	}

	std::filesystem::current_path(modelDir);
	Model model = LoadModel(modelFile.string().c_str());
	std::filesystem::current_path(originalPath);

	// apply transformation
	model.transform = transform;

	models.push_back(model);
	t_model_id id = models.size() - 1;
	loadedFromFile[key] = id;
	return id;
}

t_model_id ModelManager::loadModel(const std::string &filePath, const Vector3 &scale,
								   const Vector3 &rotation, const Vector3 &displacement)
{
	Matrix transform = getTransformMatrix(scale, rotation, displacement);
	return loadModel(filePath, transform);
}

t_model_id ModelManager::loadModel(const std::string &filePath, const Vector3 &scale)
{
	Vector3 rotation = {0.0f, 0.0f, 0.0f};
	Vector3 displacement = {0.0f, 0.0f, 0.0f};
	return loadModel(filePath, scale, rotation, displacement);
}

t_model_id ModelManager::loadModel(const std::string &filePath, float scale)
{
	Vector3 scaleVec = {scale, scale, scale};
	return loadModel(filePath, scaleVec);
}

t_model_id ModelManager::loadModel(const std::string &filePath)
{
	Matrix identityMatrix = MatrixIdentity();
	return loadModel(filePath, identityMatrix);
}

t_model_id ModelManager::createCube(float width, float height, float length)
{
	return createAndAddModel("box", [=]()
							 {
		Mesh mesh = GenMeshCube(width, height, length);
		return LoadModelFromMesh(mesh); }, width, height, length);
}

t_model_id ModelManager::createSphere(int rings, int slices, float radius)
{
	// assert(radius == 1.0);  // radius should be handled using scale
	return createAndAddModel("sphere", [=]()
							 {
		Mesh mesh = GenMeshSphere(radius, rings, slices);
		return LoadModelFromMesh(mesh); }, radius, rings, slices);
}

t_model_id ModelManager::createCylinder(int slices, float radius, float height)
{
	// assert(radius == 1.0);  // radius should be handled using scale
	return createAndAddModel("cylinder", [=]()
		{
			Mesh mesh = GenMeshCylinder(radius, height, slices);
			Model model = LoadModelFromMesh(mesh);
			Matrix transform = getTransformMatrix(
				Vector3{1.0f, 1.0f, 1.0f},			// scale
				Vector3{90.0f, 0.0f, 0.0f} * DEG2RAD,        // rotation
				Vector3{0.0f, 0.0f, -height / 2.0f}          // displacement
			);
			model.transform = transform;
			return model;
		}, radius, height, slices);
}

t_model_id ModelManager::createPlane(float width, float length, int resX, int resZ)
{
	return createAndAddModel("plane", [=]()
							 {
		Mesh mesh = GenMeshPlane(width, length, resX, resZ);
		return LoadModelFromMesh(mesh); }, width, length, resX, resZ);
}

Model &ModelManager::getModel(t_model_id id)
{
	if (!isValid(id))
	{
		throw std::out_of_range("Invalid model ID");
	}
	return models[id];
}

const Model &ModelManager::getModel(t_model_id id) const
{
	if (!isValid(id))
	{
		throw std::out_of_range("Invalid model ID");
	}
	return models[id];
}

void ModelManager::unloadAll()
{
	for (auto &model : models)
	{
		UnloadModel(model);
	}
	models.clear();
	proceduralCache.clear();
	loadedFromFile.clear();
}

bool ModelManager::isValid(t_model_id id) const
{
	return id < models.size();
}

template <typename... Args>
std::string ModelManager::generateCacheKey(const std::string &keyBase, Args &&...args) const
{
	std::stringstream ss;
	ss << std::fixed << std::setprecision(3) << keyBase;
	((ss << "_" << args), ...);
	return ss.str();
}

template <typename Func, typename... Args>
t_model_id ModelManager::createAndAddModel(const std::string &keyBase, Func modelGenerator, Args &&...args)
{
	std::string key = generateCacheKey(keyBase, args...);

	auto it = proceduralCache.find(key);
	if (it != proceduralCache.end())
	{
		return it->second;
	}

	Model model = modelGenerator(); // Call the generator function
	t_model_id id = models.size();
	models.push_back(model);
	proceduralCache[key] = id;
	return id;
}
