#include <sstream>
#include <string>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <unordered_set>
#include "rlgl.h"
#include "model_manager.hpp"
#include "utils.hpp"


ModelManager::ModelManager() {}

ModelManager::~ModelManager()
{
	unloadAll();
}

namespace {
	void prepareTexture(Texture2D &texture)
	{
		if (texture.id == 0 || texture.width <= 1 || texture.height <= 1)
			return;
		if (texture.mipmaps <= 1)
			GenTextureMipmaps(&texture);
		if (texture.mipmaps > 1)
			SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
	}

	void prepareModelTextures(Model &model)
	{
		for (int i = 0; i < model.materialCount; ++i)
		{
			Material &material = model.materials[i];
			if (material.maps == nullptr)
				continue;
			prepareTexture(material.maps[MATERIAL_MAP_DIFFUSE].texture);
			prepareTexture(material.maps[MATERIAL_MAP_NORMAL].texture);
		}
	}

	float computeModelBoundingRadius(const Model &model)
	{
		BoundingBox box = GetModelBoundingBox(model);
		float maxDistSq = 0.0f;
		for (int x : {0, 1}) {
			for (int y : {0, 1}) {
				for (int z : {0, 1}) {
					Vector3 pt = {
						x ? box.max.x : box.min.x,
						y ? box.max.y : box.min.y,
						z ? box.max.z : box.min.z
					};
					float distSq = pt.x * pt.x + pt.y * pt.y + pt.z * pt.z;
					if (distSq > maxDistSq) maxDistSq = distSq;
				}
			}
		}
		return std::max(0.01f, std::sqrt(maxDistSq));
	}
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
	prepareModelTextures(model);

	// apply transformation
	model.transform = transform;

	models.push_back(model);
	modelRadii.push_back(computeModelBoundingRadius(model));
	modelPaths.emplace_back(filePath);
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

t_model_id ModelManager::loadModel(const GameConfig& config, const std::string& configPath)
{
	std::string filePath = config.getString(configPath, "");
	return loadModel(filePath);
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

t_model_id ModelManager::createTriangle(const Vector3 &p1, const Vector3 &p2, const Vector3 &p3)
{
	return createAndAddModel("triangle", [=]()
	{
		Mesh mesh{};
		mesh.triangleCount = 2;
		mesh.vertexCount = 6;
		mesh.vertices = static_cast<float *>(MemAlloc(6 * 3 * sizeof(float)));
		mesh.normals = static_cast<float *>(MemAlloc(6 * 3 * sizeof(float)));
		mesh.texcoords = static_cast<float *>(MemAlloc(6 * 2 * sizeof(float)));

		Vector3 edge1 = p2 - p1;
		Vector3 edge2 = p3 - p1;
		Vector3 normal = Vector3CrossProduct(edge1, edge2);
		float len = Vector3Length(normal);
		if (len > 0.00001f)
			normal = normal / len;
		else
			normal = Vector3{0.0f, 0.0f, 1.0f};

		// Front face (p1, p2, p3)
		mesh.vertices[0] = p1.x; mesh.vertices[1] = p1.y; mesh.vertices[2] = p1.z;
		mesh.vertices[3] = p2.x; mesh.vertices[4] = p2.y; mesh.vertices[5] = p2.z;
		mesh.vertices[6] = p3.x; mesh.vertices[7] = p3.y; mesh.vertices[8] = p3.z;

		mesh.normals[0] = normal.x; mesh.normals[1] = normal.y; mesh.normals[2] = normal.z;
		mesh.normals[3] = normal.x; mesh.normals[4] = normal.y; mesh.normals[5] = normal.z;
		mesh.normals[6] = normal.x; mesh.normals[7] = normal.y; mesh.normals[8] = normal.z;

		mesh.texcoords[0] = 0.0f; mesh.texcoords[1] = 0.0f;
		mesh.texcoords[2] = 1.0f; mesh.texcoords[3] = 0.0f;
		mesh.texcoords[4] = 0.5f; mesh.texcoords[5] = 1.0f;

		// Back face (p1, p3, p2)
		mesh.vertices[9] = p1.x;  mesh.vertices[10] = p1.y; mesh.vertices[11] = p1.z;
		mesh.vertices[12] = p3.x; mesh.vertices[13] = p3.y; mesh.vertices[14] = p3.z;
		mesh.vertices[15] = p2.x; mesh.vertices[16] = p2.y; mesh.vertices[17] = p2.z;

		mesh.normals[9] = -normal.x;  mesh.normals[10] = -normal.y; mesh.normals[11] = -normal.z;
		mesh.normals[12] = -normal.x; mesh.normals[13] = -normal.y; mesh.normals[14] = -normal.z;
		mesh.normals[15] = -normal.x; mesh.normals[16] = -normal.y; mesh.normals[17] = -normal.z;

		mesh.texcoords[6] = 0.0f; mesh.texcoords[7] = 0.0f;
		mesh.texcoords[8] = 0.5f; mesh.texcoords[9] = 1.0f;
		mesh.texcoords[10] = 1.0f; mesh.texcoords[11] = 0.0f;

		if (IsWindowReady())
			UploadMesh(&mesh, false);
		return LoadModelFromMesh(mesh);
	}, p1, p2, p3);
}

t_model_id ModelManager::createTriangle(float rad, float angle1, float angle2, float angle3)
{
	float r = std::max(0.001f, rad);
	Vector3 p1 = { r * cosf(angle1), r * sinf(angle1), 0.0f };
	Vector3 p2 = { r * cosf(angle2), r * sinf(angle2), 0.0f };
	Vector3 p3 = { r * cosf(angle3), r * sinf(angle3), 0.0f };
	return createTriangle(p1, p2, p3);
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

std::optional<std::string> ModelManager::getModelPath(t_model_id id) const
{
	if (!isValid(id))
		throw std::out_of_range("Invalid model ID");
	return modelPaths[id];
}

void ModelManager::unloadAll()
{
	constexpr int kMaxMaterialMaps = 12;
	const bool windowReady = IsWindowReady();
	std::unordered_set<unsigned int> unloadedTextureIds;

	for (auto &model : models)
	{
		if (model.materials != nullptr)
		{
			for (int i = 0; i < model.materialCount; ++i)
			{
				Material &material = model.materials[i];
				if (material.maps != nullptr && windowReady)
				{
					for (int j = 0; j < kMaxMaterialMaps; ++j)
					{
						Texture2D &texture = material.maps[j].texture;
						if (texture.id > 0 && texture.id != rlGetTextureIdDefault())
						{
							if (unloadedTextureIds.insert(texture.id).second)
								UnloadTexture(texture);
							texture.id = 0;
						}
					}
				}
				material.shader = {0, nullptr};
			}
		}

		if (model.meshCount > 0 && model.meshes != nullptr)
			UnloadModel(model);
	}
	models.clear();
	modelRadii.clear();
	modelPaths.clear();
	proceduralCache.clear();
	loadedFromFile.clear();
}

bool ModelManager::isValid(t_model_id id) const
{
	return id < models.size();
}

float ModelManager::getModelRadius(t_model_id id) const
{
	if (id < modelRadii.size())
		return modelRadii[id];
	return 1.0f;
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
	modelRadii.push_back(computeModelBoundingRadius(model));
	modelPaths.emplace_back(std::nullopt);
	proceduralCache[key] = id;
	return id;
}
