#include "collision_body_manager.hpp"
#include "game_context.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

namespace
{
	BoundingBox getTriangleBounds(const CollisionTriangle &triangle)
	{
		BoundingBox result{triangle.a, triangle.a};
		result.min = Vector3Min(result.min, triangle.b);
		result.min = Vector3Min(result.min, triangle.c);
		result.max = Vector3Max(result.max, triangle.b);
		result.max = Vector3Max(result.max, triangle.c);
		return result;
	}

	BoundingBox mergeBounds(const BoundingBox &left, const BoundingBox &right)
	{
		return BoundingBox{
			Vector3Min(left.min, right.min),
			Vector3Max(left.max, right.max)
		};
	}

	Vector3 getTriangleCentroid(const CollisionTriangle &triangle)
	{
		return Vector3{
			(triangle.a.x + triangle.b.x + triangle.c.x) / 3.0f,
			(triangle.a.y + triangle.b.y + triangle.c.y) / 3.0f,
			(triangle.a.z + triangle.b.z + triangle.c.z) / 3.0f
		};
	}

	int getLargestAxis(const Vector3 &extent)
	{
		if (extent.x >= extent.y && extent.x >= extent.z)
			return 0;
		if (extent.y >= extent.z)
			return 1;
		return 2;
	}

	float getAxisValue(const Vector3 &value, int axis)
	{
		if (axis == 0)
			return value.x;
		if (axis == 1)
			return value.y;
		return value.z;
	}

	BoundingBox getRangeBounds(
		const std::vector<CollisionTriangle> &triangles,
		std::size_t firstTriangle,
		std::size_t triangleCount
	)
	{
		BoundingBox result = getTriangleBounds(triangles[firstTriangle]);
		for (std::size_t offset = 1; offset < triangleCount; ++offset)
			result = mergeBounds(result, getTriangleBounds(triangles[firstTriangle + offset]));
		return result;
	}

	BoundingBox getCentroidBounds(
		const std::vector<CollisionTriangle> &triangles,
		std::size_t firstTriangle,
		std::size_t triangleCount
	)
	{
		BoundingBox result{
			getTriangleCentroid(triangles[firstTriangle]),
			getTriangleCentroid(triangles[firstTriangle])
		};
		for (std::size_t offset = 1; offset < triangleCount; ++offset)
		{
			const Vector3 centroid = getTriangleCentroid(triangles[firstTriangle + offset]);
			result.min = Vector3Min(result.min, centroid);
			result.max = Vector3Max(result.max, centroid);
		}
		return result;
	}

	int buildBvhNode(
		CollisionModel &model,
		std::size_t firstTriangle,
		std::size_t triangleCount
	)
	{
		const int nodeIndex = static_cast<int>(model.bvh.size());
		model.bvh.push_back(CollisionBvhNode{});
		model.bvh[nodeIndex].bounds = getRangeBounds(
			model.triangles,
			firstTriangle,
			triangleCount
		);

		constexpr std::size_t leafTriangleCount = 4;
		if (triangleCount <= leafTriangleCount)
		{
			model.bvh[nodeIndex].firstTriangle = firstTriangle;
			model.bvh[nodeIndex].triangleCount = triangleCount;
			return nodeIndex;
		}

		const BoundingBox centroidBounds = getCentroidBounds(
			model.triangles,
			firstTriangle,
			triangleCount
		);
		const int splitAxis = getLargestAxis(centroidBounds.max - centroidBounds.min);
		const std::size_t middleTriangle = firstTriangle + triangleCount / 2;

		std::nth_element(
			model.triangles.begin() + static_cast<std::ptrdiff_t>(firstTriangle),
			model.triangles.begin() + static_cast<std::ptrdiff_t>(middleTriangle),
			model.triangles.begin() + static_cast<std::ptrdiff_t>(firstTriangle + triangleCount),
			[splitAxis](const CollisionTriangle &left, const CollisionTriangle &right)
			{
				return getAxisValue(getTriangleCentroid(left), splitAxis)
					< getAxisValue(getTriangleCentroid(right), splitAxis);
			}
		);

		const int leftChild = buildBvhNode(
			model,
			firstTriangle,
			middleTriangle - firstTriangle
		);
		const int rightChild = buildBvhNode(
			model,
			middleTriangle,
			firstTriangle + triangleCount - middleTriangle
		);
		model.bvh[nodeIndex].leftChild = leftChild;
		model.bvh[nodeIndex].rightChild = rightChild;
		return nodeIndex;
	}

	std::filesystem::path getCollisionPath(const std::string &modelPath)
	{
		std::filesystem::path collisionPath(modelPath);
		collisionPath.replace_extension(".collision.obj");
		return collisionPath;
	}

	BoundingBox transformBounds(const BoundingBox &bounds, const Matrix &transform)
	{
		const std::array<Vector3, 8> corners{
			Vector3{bounds.min.x, bounds.min.y, bounds.min.z},
			Vector3{bounds.max.x, bounds.min.y, bounds.min.z},
			Vector3{bounds.min.x, bounds.max.y, bounds.min.z},
			Vector3{bounds.max.x, bounds.max.y, bounds.min.z},
			Vector3{bounds.min.x, bounds.min.y, bounds.max.z},
			Vector3{bounds.max.x, bounds.min.y, bounds.max.z},
			Vector3{bounds.min.x, bounds.max.y, bounds.max.z},
			Vector3{bounds.max.x, bounds.max.y, bounds.max.z}
		};
		const Vector3 firstCorner = Vector3Transform(corners[0], transform);
		BoundingBox result{firstCorner, firstCorner};

		for (std::size_t index = 1; index < corners.size(); ++index)
		{
			const Vector3 transformedCorner = Vector3Transform(corners[index], transform);
			result.min = Vector3Min(result.min, transformedCorner);
			result.max = Vector3Max(result.max, transformedCorner);
		}
		return result;
	}
}

t_collision_mesh_id CollisionBodyManager::loadCollisionModel(const std::string &path)
{
	return loadCollisionModel(std::filesystem::path(path));
}

t_collision_mesh_id CollisionBodyManager::loadCollisionModel(const char *path)
{
	if (path == nullptr)
		throw std::invalid_argument("Collision model path cannot be null");
	return loadCollisionModel(std::filesystem::path(path));
}

t_collision_mesh_id CollisionBodyManager::loadCollisionModel(
	const std::filesystem::path &path
)
{
	return loadCollisionModel(path, MatrixIdentity());
}

t_collision_mesh_id CollisionBodyManager::loadCollisionModel(
	const ModelManager &modelManager,
	t_model_id modelID
)
{
	const Model &model = modelManager.getModel(modelID);
	const std::optional<std::string> modelPath = modelManager.getModelPath(modelID);

	if (modelPath)
		return loadCollisionModel(getCollisionPath(*modelPath), model.transform);

	const std::pair<t_model_id, Matrix> key{modelID, model.transform};
	const std::map<std::pair<t_model_id, Matrix>, t_collision_mesh_id>::const_iterator iterator =
		loadedFromModel.find(key);
	if (iterator != loadedFromModel.end())
		return iterator->second;

	const t_collision_mesh_id id = addCollisionModel(
		copyCollisionMesh(model),
		model.transform
	);
	loadedFromModel[key] = id;
	return id;
}

t_collision_mesh_id CollisionBodyManager::loadCollisionModel(
	const GameContext &context,
	t_model_id modelID
)
{
	return loadCollisionModel(context.modelManager, modelID);
}

t_collision_mesh_id CollisionBodyManager::loadCollisionModel(
	const std::filesystem::path &path,
	const Matrix &staticTransform
)
{
	const std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(path);
	const std::pair<std::string, Matrix> key{canonicalPath.string(), staticTransform};
	const std::map<std::pair<std::string, Matrix>, t_collision_mesh_id>::const_iterator iterator =
		loadedFromFile.find(key);
	if (iterator != loadedFromFile.end())
		return iterator->second;

	const t_collision_mesh_id id = addCollisionModel(
		loadCollisionMesh(canonicalPath),
		staticTransform
	);
	loadedFromFile[key] = id;
	return id;
}

t_collision_mesh_id CollisionBodyManager::addCollisionModel(
	CollisionMeshData mesh,
	const Matrix &staticTransform
)
{
	CollisionModel model;
	model.triangles = std::move(mesh.triangles);
	model.bounds = mesh.bounds;
	model.boundingRadius = mesh.boundingRadius;
	model.staticTransform = staticTransform;
	buildBvh(model);

	const t_collision_mesh_id id = models.size();
	models.push_back(std::move(model));
	return id;
}

void CollisionBodyManager::buildBvh(CollisionModel &model)
{
	model.bvh.clear();
	if (!model.triangles.empty())
		buildBvhNode(model, 0, model.triangles.size());
}

const CollisionModel &CollisionBodyManager::getCollisionModel(t_collision_mesh_id id) const
{
	if (!isValid(id))
		throw std::out_of_range("Invalid collision mesh ID");
	return models[id];
}

float CollisionBodyManager::getCollisionRadius(
	t_collision_mesh_id id,
	const Vector3 &translation,
	const Vector3 &scale,
	const Quaternion &rotation
) const
{
	const CollisionModel &model = getCollisionModel(id);
	if (model.triangles.empty())
		return 0.0f;

	const Matrix modelTransform = MatrixMultiply(
		model.staticTransform,
		MatrixMultiply(
			MatrixScale(scale.x, scale.y, scale.z),
			QuaternionToMatrix(rotation)
		)
	);
	const BoundingBox transformedBounds = transformBounds(model.bounds, modelTransform);
	const Vector3 rotatedTranslation = Vector3RotateByQuaternion(translation, rotation);
	const std::array<Vector3, 8> corners{
		Vector3{transformedBounds.min.x, transformedBounds.min.y, transformedBounds.min.z},
		Vector3{transformedBounds.max.x, transformedBounds.min.y, transformedBounds.min.z},
		Vector3{transformedBounds.min.x, transformedBounds.max.y, transformedBounds.min.z},
		Vector3{transformedBounds.max.x, transformedBounds.max.y, transformedBounds.min.z},
		Vector3{transformedBounds.min.x, transformedBounds.min.y, transformedBounds.max.z},
		Vector3{transformedBounds.max.x, transformedBounds.min.y, transformedBounds.max.z},
		Vector3{transformedBounds.min.x, transformedBounds.max.y, transformedBounds.max.z},
		Vector3{transformedBounds.max.x, transformedBounds.max.y, transformedBounds.max.z}
	};

	float radius = 0.0f;
	for (const Vector3 &corner : corners)
	{
		const Vector3 translatedCorner = Vector3Add(corner, rotatedTranslation);
		radius = std::max(radius, Vector3Length(translatedCorner));
	}
	return radius;
}

bool CollisionBodyManager::isValid(t_collision_mesh_id id) const
{
	return id < models.size();
}

void CollisionBodyManager::unloadAll()
{
	models.clear();
	loadedFromFile.clear();
	loadedFromModel.clear();
}
