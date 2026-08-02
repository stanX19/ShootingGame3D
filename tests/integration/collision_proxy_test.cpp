#include "catch2/catch_amalgamated.hpp"

#include "collision_algorithm.hpp"
#include "collision_body_manager.hpp"
#include "utils.hpp"

#include <filesystem>
#include <fstream>
#include <cmath>
#include <string>

namespace
{
	class TemporaryObj
	{
	public:
		TemporaryObj(const std::string &fileName, const std::string &contents)
			: path(std::filesystem::temp_directory_path() / fileName)
		{
			std::ofstream output(path);
			REQUIRE(output.good());
			output << contents;
		}

		~TemporaryObj()
		{
			std::error_code error;
			std::filesystem::remove(path, error);
		}

		std::filesystem::path path;
	};

	const std::string squareObj =
		"v -1 -1 0\n"
		"v 1 -1 0\n"
		"v 1 1 0\n"
		"v -1 1 0\n"
		"f 1 2 3\n"
		"f 1 3 4\n";

	const CollisionModel &loadSquare(CollisionBodyManager &manager, const std::filesystem::path &path)
	{
		t_collision_mesh_id modelID = manager.loadCollisionModel(path);
		return manager.getCollisionModel(modelID);
	}
}

TEST_CASE("Adjacent square corners collide while diagonal corners miss", "[collision-proxy]")
{
	const Vector3 squareOrigin = Vector3Zeros;
	const Vector3 adjacentCorner = {2.0f, 0.0f, 0.0f};
	const Vector3 diagonalCorner = {2.0f, 0.0f, 2.0f};
	const float sphereCollisionDistance = 2.0f;

	REQUIRE(willCollide(
		squareOrigin,
		Vector3Zeros,
		adjacentCorner,
		Vector3Zeros,
		sphereCollisionDistance,
		1.0f
	));
	REQUIRE_FALSE(willCollide(
		squareOrigin,
		Vector3Zeros,
		diagonalCorner,
		Vector3Zeros,
		sphereCollisionDistance,
		1.0f
	));
}

TEST_CASE("Expanded square corners remain outside the sphere collision radius", "[collision-proxy]")
{
	const Vector3 squareOrigin = Vector3Zeros;
	const Vector3 expandedAdjacentCorner = {2.1f, 0.0f, 0.0f};

	REQUIRE_FALSE(willCollide(
		squareOrigin,
		Vector3Zeros,
		expandedAdjacentCorner,
		Vector3Zeros,
		2.0f,
		1.0f
	));
}

TEST_CASE("Swept sphere hits and misses the square collision proxy intentionally", "[collision-proxy]")
{
	TemporaryObj collisionObj("shooting_game_collision_proxy_square.obj", squareObj);
	CollisionBodyManager manager;
	const CollisionModel &square = loadSquare(manager, collisionObj.path);
	const CollisionMeshInstance stationarySquare{
		Vector3Zeros,
		Vector3Zeros,
		Vector3Zeros,
		Vector3{1.0f, 1.0f, 1.0f},
		QuaternionIdentity()
	};
	const CollisionInterval fullFrame{0.0f, 1.0f};

	std::optional<CollisionHit> hit = sweepSphereAgainstMesh(
		square,
		stationarySquare,
		Vector3{0.0f, 0.0f, 2.0f},
		Vector3{0.0f, 0.0f, -3.0f},
		0.25f,
		fullFrame
	);
	REQUIRE(hit);
	REQUIRE(hit->collisionDt == Catch::Approx(1.75f / 3.0f));

	std::optional<CollisionHit> miss = sweepSphereAgainstMesh(
		square,
		stationarySquare,
		Vector3{2.0f, 2.0f, 2.0f},
		Vector3{0.0f, 0.0f, -3.0f},
		0.25f,
		fullFrame
	);
	REQUIRE_FALSE(miss);
}

TEST_CASE("Collision proxy radius includes render translation and scale", "[collision-proxy]")
{
	TemporaryObj collisionObj("shooting_game_collision_proxy_radius.obj", squareObj);
	CollisionBodyManager manager;
	const t_collision_mesh_id squareID = manager.loadCollisionModel(collisionObj.path);
	const CollisionMeshInstance transformedSquare{
		Vector3Zeros,
		Vector3Zeros,
		Vector3{2.0f, 0.0f, 0.0f},
		Vector3{2.0f, 1.0f, 1.0f},
		QuaternionIdentity()
	};

	REQUIRE(manager.getCollisionRadius(
		squareID,
		transformedSquare.translation,
		transformedSquare.scale,
		transformedSquare.rotation
	) == Catch::Approx(std::sqrt(17.0f)));
}
