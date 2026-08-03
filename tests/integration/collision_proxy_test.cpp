#include "catch2/catch_amalgamated.hpp"

#include "collision_algorithm.hpp"
#include "collision_body_manager.hpp"
#include "utils.hpp"

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
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

	const std::string octahedronObj =
		"v 1 0 0\n"
		"v -1 0 0\n"
		"v 0 1 0\n"
		"v 0 -1 0\n"
		"v 0 0 1\n"
		"v 0 0 -1\n"
		"f 1 3 5\n"
		"f 1 5 4\n"
		"f 1 4 6\n"
		"f 1 6 3\n"
		"f 2 5 3\n"
		"f 2 3 6\n"
		"f 2 6 4\n"
		"f 2 4 5\n";

	void appendFace(std::ostringstream &output, int a, int b, int c)
	{
		output << "f " << a << ' ' << b << ' ' << c << '\n';
	}

	std::string makeSquareDonutObj()
	{
		const float outerRadius = 2.0f;
		const float innerRadius = 0.75f;
		const float halfDepth = 0.75f;
		const std::array<Vector3, 16> vertices{
			Vector3{-outerRadius, -outerRadius, -halfDepth},
			Vector3{outerRadius, -outerRadius, -halfDepth},
			Vector3{outerRadius, outerRadius, -halfDepth},
			Vector3{-outerRadius, outerRadius, -halfDepth},
			Vector3{-outerRadius, -outerRadius, halfDepth},
			Vector3{outerRadius, -outerRadius, halfDepth},
			Vector3{outerRadius, outerRadius, halfDepth},
			Vector3{-outerRadius, outerRadius, halfDepth},
			Vector3{-innerRadius, -innerRadius, -halfDepth},
			Vector3{innerRadius, -innerRadius, -halfDepth},
			Vector3{innerRadius, innerRadius, -halfDepth},
			Vector3{-innerRadius, innerRadius, -halfDepth},
			Vector3{-innerRadius, -innerRadius, halfDepth},
			Vector3{innerRadius, -innerRadius, halfDepth},
			Vector3{innerRadius, innerRadius, halfDepth},
			Vector3{-innerRadius, innerRadius, halfDepth}
		};
		const int outerBottomStart = 1;
		const int outerTopStart = 5;
		const int innerBottomStart = 9;
		const int innerTopStart = 13;
		std::ostringstream output;

		for (const Vector3 &vertex : vertices)
			output << "v " << vertex.x << ' ' << vertex.y << ' ' << vertex.z << '\n';

		for (int side = 0; side < 4; ++side)
		{
			const int nextSide = (side + 1) % 4;

			appendFace(output, outerTopStart + side, outerTopStart + nextSide, innerTopStart + nextSide);
			appendFace(output, outerTopStart + side, innerTopStart + nextSide, innerTopStart + side);
			appendFace(output, outerBottomStart + side, innerBottomStart + nextSide, outerBottomStart + nextSide);
			appendFace(output, outerBottomStart + side, innerBottomStart + side, innerBottomStart + nextSide);

			appendFace(output, outerBottomStart + side, outerBottomStart + nextSide, outerTopStart + nextSide);
			appendFace(output, outerBottomStart + side, outerTopStart + nextSide, outerTopStart + side);
			appendFace(output, innerBottomStart + side, innerTopStart + side, innerTopStart + nextSide);
			appendFace(output, innerBottomStart + side, innerTopStart + nextSide, innerBottomStart + nextSide);
		}

		return output.str();
	}

	const CollisionModel &loadMesh(CollisionBodyManager &manager, const std::filesystem::path &path)
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
	const CollisionModel &square = loadMesh(manager, collisionObj.path);
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

TEST_CASE("Sphere fully inside a closed collision mesh reports an initial collision", "[collision-proxy]")
{
	TemporaryObj collisionObj("shooting_game_collision_proxy_octahedron.obj", octahedronObj);
	CollisionBodyManager manager;
	const CollisionModel &octahedron = loadMesh(manager, collisionObj.path);
	const CollisionMeshInstance stationaryMesh{
		Vector3Zeros,
		Vector3Zeros,
		Vector3Zeros,
		Vector3{1.0f, 1.0f, 1.0f},
		QuaternionIdentity()
	};

	const std::optional<CollisionHit> hit = sweepSphereAgainstMesh(
		octahedron,
		stationaryMesh,
		Vector3Zeros,
		Vector3Zeros,
		0.1f,
		CollisionInterval{0.0f, 1.0f}
	);

	REQUIRE(hit);
	REQUIRE(hit->collisionDt == Catch::Approx(0.0f));
}

TEST_CASE("Containment respects a non-convex collision mesh hole", "[collision-proxy]")
{
	TemporaryObj collisionObj("shooting_game_collision_proxy_donut.obj", makeSquareDonutObj());
	CollisionBodyManager manager;
	const CollisionModel &donut = loadMesh(manager, collisionObj.path);
	const CollisionMeshInstance stationaryMesh{
		Vector3Zeros,
		Vector3Zeros,
		Vector3Zeros,
		Vector3{1.0f, 1.0f, 1.0f},
		QuaternionIdentity()
	};
	const CollisionInterval fullFrame{0.0f, 1.0f};

	const std::optional<CollisionHit> holeHit = sweepSphereAgainstMesh(
		donut,
		stationaryMesh,
		Vector3Zeros,
		Vector3Zeros,
		0.1f,
		fullFrame
	);
	REQUIRE_FALSE(holeHit);

	const std::optional<CollisionHit> solidHit = sweepSphereAgainstMesh(
		donut,
		stationaryMesh,
		Vector3{1.3f, 0.0f, 0.0f},
		Vector3Zeros,
		0.1f,
		fullFrame
	);
	REQUIRE(solidHit);
	REQUIRE(solidHit->collisionDt == Catch::Approx(0.0f));
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
