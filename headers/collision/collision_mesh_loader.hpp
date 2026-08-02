#ifndef COLLISION_MESH_LOADER_HPP
#define COLLISION_MESH_LOADER_HPP

#include "includes.hpp"

#include <filesystem>
#include <vector>

struct CollisionTriangle
{
	Vector3 a;
	Vector3 b;
	Vector3 c;
};

struct CollisionMeshData
{
	std::vector<CollisionTriangle> triangles;
	BoundingBox bounds;
	float boundingRadius = 0.0f;
};

CollisionMeshData loadCollisionMesh(const std::filesystem::path &path);
CollisionMeshData copyCollisionMesh(const Model &model);

#endif
