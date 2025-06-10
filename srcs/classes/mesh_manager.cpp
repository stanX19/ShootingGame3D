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

	Model model = LoadModel(filePath.c_str());
	models.push_back(model);
	t_mesh_id id = models.size() - 1;
	loadedFromFile[filePath] = id;
	return id;
}

t_mesh_id MeshManager::createBox(float width, float height, float length)
{
	t_mesh_id cachedID;
	if (paramExists("box", cachedID, width, height, length))
		return cachedID;

	Mesh mesh = GenMeshCube(width, height, length);
	Model model = LoadModelFromMesh(mesh);
	models.push_back(model);
	return models.size() - 1;
}

t_mesh_id MeshManager::createSphere(float radius, int rings, int slices)
{
	t_mesh_id cachedID;
	if (paramExists("sphere", cachedID, radius, rings, slices))
		return cachedID;

	Mesh mesh = GenMeshSphere(radius, rings, slices);
	Model model = LoadModelFromMesh(mesh);
	models.push_back(model);
	return models.size() - 1;
}

t_mesh_id MeshManager::createPlane(float width, float length, int resX, int resZ)
{
	t_mesh_id cachedID;
	if (paramExists("plane", cachedID, width, length, resX, resZ))
		return cachedID;

	Mesh mesh = GenMeshPlane(width, length, resX, resZ);
	Model model = LoadModelFromMesh(mesh);
	models.push_back(model);
	return models.size() - 1;
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
	loadedFromFile.clear();
}

bool MeshManager::isValid(t_mesh_id id) const
{
	return id < models.size();
}

template <typename... Args>
bool MeshManager::paramExists(const std::string &keyBase, t_mesh_id &outId, Args &&...args)
{
	std::stringstream ss;
	
	ss << std::fixed << std::setprecision(3);
	ss << keyBase;

	// Expands to (ss << "_" << arg1), (ss << "_" << arg2), (ss << "_" << arg3)
	((ss << "_" << args), ...);

	std::string key = ss.str();

	auto it = proceduralCache.find(key);
	if (it != proceduralCache.end())
	{
		outId = it->second;
		return true;
	}

	return false;
}
