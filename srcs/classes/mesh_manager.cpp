#include <sstream>
#include <string>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <iostream>
#include "mesh_manager.hpp"

MeshManager::MeshManager() {}

MeshManager::~MeshManager()
{
	unloadAll();
}


t_mesh_id MeshManager::loadModel(const std::string &filePath)
{
	auto it = loadedFromFile.find(filePath);
	if (it != loadedFromFile.end())
	{
		return it->second;
	}

	std::string orginalPath = std::filesystem::current_path();
	std::filesystem::path modelPath = std::filesystem::path(filePath);
	
	// go into the directory for reading, then exit
	std::filesystem::current_path(modelPath.parent_path());
	Model model = LoadModel(modelPath.filename().c_str());
	std::filesystem::current_path(orginalPath);

	models.push_back(model);
	t_mesh_id id = models.size() - 1;
	loadedFromFile[filePath] = id;
	return id;
}

t_mesh_id MeshManager::createBox(float width, float height, float length)
{
	return createAndAddModel("box", [=]()
							 {
		Mesh mesh = GenMeshCube(width, height, length);
		return LoadModelFromMesh(mesh); }, width, height, length);
}

t_mesh_id MeshManager::createSphere(float radius, int rings, int slices)
{
	return createAndAddModel("sphere", [=]()
							 {
		Mesh mesh = GenMeshSphere(radius, rings, slices);
		return LoadModelFromMesh(mesh); }, radius, rings, slices);
}

t_mesh_id MeshManager::createPlane(float width, float length, int resX, int resZ)
{
	return createAndAddModel("plane", [=]()
							 {
		Mesh mesh = GenMeshPlane(width, length, resX, resZ);
		return LoadModelFromMesh(mesh); }, width, length, resX, resZ);
}

Model &MeshManager::getModel(t_mesh_id id)
{
	if (!isValid(id))
	{
		throw std::out_of_range("Invalid model ID");
	}
	return models[id];
}

const Model &MeshManager::getModel(t_mesh_id id) const
{
	if (!isValid(id))
	{
		throw std::out_of_range("Invalid model ID");
	}
	return models[id];
}

void MeshManager::unloadAll()
{
	for (auto &model : models)
	{
		UnloadModel(model);
	}
	models.clear();
	proceduralCache.clear();
	loadedFromFile.clear();
}

bool MeshManager::isValid(t_mesh_id id) const
{
	return id < models.size();
}

template <typename... Args>
std::string MeshManager::generateCacheKey(const std::string &keyBase, Args &&...args) const
{
	std::stringstream ss;
	ss << std::fixed << std::setprecision(3) << keyBase;
	((ss << "_" << args), ...);
	return ss.str();
}

template <typename Func, typename... Args>
t_mesh_id MeshManager::createAndAddModel(const std::string &keyBase, Func modelGenerator, Args &&...args)
{
	std::string key = generateCacheKey(keyBase, args...);

	auto it = proceduralCache.find(key);
	if (it != proceduralCache.end())
	{
		return it->second;
	}

	Model model = modelGenerator(); // Call the generator function
	t_mesh_id id = models.size();
	models.push_back(model);
	proceduralCache[key] = id;
	return id;
}
