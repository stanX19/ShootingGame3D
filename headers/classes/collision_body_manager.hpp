#ifndef COLLISION_BODY_MANAGER_HPP
#define COLLISION_BODY_MANAGER_HPP

#include "collision_mesh_loader.hpp"
#include "model_manager.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <vector>

using t_collision_mesh_id = std::size_t;

struct GameContext;

struct CollisionBvhNode
{
	BoundingBox bounds;
	std::size_t firstTriangle = 0;
	std::size_t triangleCount = 0;
	int leftChild = -1;
	int rightChild = -1;
};

struct CollisionModel
{
	std::vector<CollisionTriangle> triangles;
	std::vector<CollisionBvhNode> bvh;
	BoundingBox bounds;
	float boundingRadius = 0.0f;
	Matrix staticTransform = MatrixIdentity();
};

class CollisionBodyManager
{
public:
	CollisionBodyManager() = default;
	~CollisionBodyManager() = default;

	t_collision_mesh_id loadCollisionModel(const std::filesystem::path &path);
	t_collision_mesh_id loadCollisionModel(const std::string &path);
	t_collision_mesh_id loadCollisionModel(const char *path);
	t_collision_mesh_id loadCollisionModel(const ModelManager &modelManager, t_model_id modelID);
	t_collision_mesh_id loadCollisionModel(const GameContext &context, t_model_id modelID);

	const CollisionModel &getCollisionModel(t_collision_mesh_id id) const;
	float getCollisionRadius(
		t_collision_mesh_id id,
		const Vector3 &translation,
		const Vector3 &scale,
		const Quaternion &rotation
	) const;

	bool isValid(t_collision_mesh_id id) const;
	void unloadAll();

private:
	t_collision_mesh_id loadCollisionModel(
		const std::filesystem::path &path,
		const Matrix &staticTransform
	);
	t_collision_mesh_id addCollisionModel(CollisionMeshData mesh, const Matrix &staticTransform);
	void buildBvh(CollisionModel &model);

	std::vector<CollisionModel> models;
	std::map<std::pair<std::string, Matrix>, t_collision_mesh_id> loadedFromFile;
	std::map<std::pair<t_model_id, Matrix>, t_collision_mesh_id> loadedFromModel;
};

#endif
