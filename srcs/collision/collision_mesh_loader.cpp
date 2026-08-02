#include "collision_mesh_loader.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{
	void addPointToBounds(CollisionMeshData &mesh, const Vector3 &point)
	{
		mesh.bounds.min = Vector3Min(mesh.bounds.min, point);
		mesh.bounds.max = Vector3Max(mesh.bounds.max, point);
		mesh.boundingRadius = std::max(mesh.boundingRadius, Vector3Length(point));
	}

	void addTriangle(
		CollisionMeshData &mesh,
		const Vector3 &a,
		const Vector3 &b,
		const Vector3 &c
	)
	{
		const bool firstTriangle = mesh.triangles.empty();
		mesh.triangles.push_back(CollisionTriangle{a, b, c});
		if (firstTriangle)
		{
			mesh.bounds.min = a;
			mesh.bounds.max = a;
		}
		addPointToBounds(mesh, a);
		addPointToBounds(mesh, b);
		addPointToBounds(mesh, c);
	}

	int parseObjIndex(const std::string &token, std::size_t vertexCount)
	{
		const std::size_t slashPosition = token.find('/');
		const std::string positionToken = token.substr(0, slashPosition);
		if (positionToken.empty())
			throw std::runtime_error("Malformed collision OBJ face vertex");

		int rawIndex = 0;
		try
		{
			size_t parsedCharacters = 0;
			rawIndex = std::stoi(positionToken, &parsedCharacters);
			if (parsedCharacters != positionToken.size())
				throw std::runtime_error("trailing characters");
		}
		catch (const std::exception &)
		{
			throw std::runtime_error("Invalid collision OBJ vertex index");
		}

		if (rawIndex == 0)
			throw std::runtime_error("Collision OBJ vertex index cannot be zero");

		const int resolvedIndex = rawIndex > 0
			? rawIndex - 1
			: static_cast<int>(vertexCount) + rawIndex;
		if (resolvedIndex < 0 || resolvedIndex >= static_cast<int>(vertexCount))
			throw std::runtime_error("Collision OBJ vertex index is out of range");

		return resolvedIndex;
	}

	Vector3 getMeshVertex(const Mesh &mesh, int vertexIndex)
	{
		const int offset = vertexIndex * 3;
		return Vector3{
			mesh.vertices[offset],
			mesh.vertices[offset + 1],
			mesh.vertices[offset + 2]
		};
	}
}

CollisionMeshData loadCollisionMesh(const std::filesystem::path &path)
{
	if (!std::filesystem::is_regular_file(path))
		throw std::runtime_error("Collision model does not exist: " + path.string());

	std::ifstream input(path);
	if (!input)
		throw std::runtime_error("Cannot open collision model: " + path.string());

	CollisionMeshData result;
	std::vector<Vector3> vertices;
	std::string line;

	while (std::getline(input, line))
	{
		std::istringstream stream(line);
		std::string record;
		stream >> record;

		if (record.empty() || record.front() == '#')
			continue;

		if (record == "v")
		{
			Vector3 vertex;
			if (!(stream >> vertex.x >> vertex.y >> vertex.z))
				throw std::runtime_error("Malformed collision OBJ vertex record");
			vertices.push_back(vertex);
			continue;
		}

		if (record != "f")
			continue;

		std::array<std::string, 3> face;
		for (std::string &token : face)
		{
			if (!(stream >> token))
				throw std::runtime_error("Only triangular collision OBJ faces are supported");
		}

		std::string extraToken;
		if (stream >> extraToken && extraToken.front() != '#')
			throw std::runtime_error("Only triangular collision OBJ faces are supported");

		addTriangle(
			result,
			vertices[parseObjIndex(face[0], vertices.size())],
			vertices[parseObjIndex(face[1], vertices.size())],
			vertices[parseObjIndex(face[2], vertices.size())]
		);
	}

	if (result.triangles.empty())
		throw std::runtime_error("Collision OBJ contains no geometry: " + path.string());

	return result;
}

CollisionMeshData copyCollisionMesh(const Model &model)
{
	CollisionMeshData result;
	if (model.meshes == nullptr)
		throw std::runtime_error("Render model contains no mesh data");

	for (int meshIndex = 0; meshIndex < model.meshCount; ++meshIndex)
	{
		const Mesh &mesh = model.meshes[meshIndex];
		if (mesh.vertices == nullptr || mesh.vertexCount <= 0)
			continue;

		if (mesh.indices != nullptr)
		{
			if (mesh.triangleCount <= 0)
				continue;
			for (int triangleIndex = 0; triangleIndex < mesh.triangleCount; ++triangleIndex)
			{
				const int indexOffset = triangleIndex * 3;
				addTriangle(
					result,
					getMeshVertex(mesh, mesh.indices[indexOffset]),
					getMeshVertex(mesh, mesh.indices[indexOffset + 1]),
					getMeshVertex(mesh, mesh.indices[indexOffset + 2])
				);
			}
			continue;
		}

		if (mesh.vertexCount % 3 != 0)
			throw std::runtime_error("Render mesh does not contain triangular geometry");

		for (int vertexIndex = 0; vertexIndex < mesh.vertexCount; vertexIndex += 3)
		{
			addTriangle(
				result,
				getMeshVertex(mesh, vertexIndex),
				getMeshVertex(mesh, vertexIndex + 1),
				getMeshVertex(mesh, vertexIndex + 2)
			);
		}
	}

	if (result.triangles.empty())
		throw std::runtime_error("Render model contains no collision geometry");

	return result;
}
