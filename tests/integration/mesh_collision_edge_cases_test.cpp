#include "catch2/catch_amalgamated.hpp"

#include "collision_algorithm.hpp"
#include "collision_body_manager.hpp"
#include "utils.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

namespace
{
	class TemporaryMeshFile
	{
	public:
		TemporaryMeshFile(const std::string &filename, const std::string &content)
			: path(std::filesystem::temp_directory_path() / filename)
		{
			std::ofstream out(path);
			REQUIRE(out.good());
			out << content;
		}

		~TemporaryMeshFile()
		{
			std::error_code ec;
			std::filesystem::remove(path, ec);
		}

		std::filesystem::path path;
	};

	// Cube centered at origin from -1 to 1 on all axes (size 2x2x2)
	const std::string unitCubeObj =
		"v -1 -1 -1\n"
		"v  1 -1 -1\n"
		"v  1  1 -1\n"
		"v -1  1 -1\n"
		"v -1 -1  1\n"
		"v  1 -1  1\n"
		"v  1  1  1\n"
		"v -1  1  1\n"
		// Front face (z = 1)
		"f 5 6 7\n"
		"f 5 7 8\n"
		// Back face (z = -1)
		"f 2 1 4\n"
		"f 2 4 3\n"
		// Top face (y = 1)
		"f 4 8 7\n"
		"f 4 7 3\n"
		// Bottom face (y = -1)
		"f 1 5 6\n"
		"f 1 6 2\n"
		// Right face (x = 1)
		"f 6 2 3\n"
		"f 6 3 7\n"
		// Left face (x = -1)
		"f 1 5 8\n"
		"f 1 8 4\n";

	const CollisionMeshInstance stationaryIdentity{
		Vector3Zeros,
		Vector3Zeros,
		Vector3Zeros,
		Vector3{1.0f, 1.0f, 1.0f},
		QuaternionIdentity()
	};

	const CollisionInterval standardFrame{0.0f, 1.0f};
}

TEST_CASE("Collision in model: sphere fully inside closed cube reports collision at dt 0", "[integration][collision][mesh]")
{
	TemporaryMeshFile cubeFile("unit_test_cube_inside.obj", unitCubeObj);
	CollisionBodyManager manager;
	const t_collision_mesh_id cubeId = manager.loadCollisionModel(cubeFile.path);
	const CollisionModel &cube = manager.getCollisionModel(cubeId);

	// Sphere at center (0, 0, 0) with radius 0.5, stationary
	const std::optional<CollisionHit> hit = sweepSphereAgainstMesh(
		cube,
		stationaryIdentity,
		Vector3Zeros,
		Vector3Zeros,
		0.5f,
		standardFrame
	);

	REQUIRE(hit.has_value());
	REQUIRE(hit->collisionDt == Catch::Approx(0.0f));
}

TEST_CASE("Collision in model: sphere moving completely inside closed cube reports collision at dt 0", "[integration][collision][mesh]")
{
	TemporaryMeshFile cubeFile("unit_test_cube_inside_moving.obj", unitCubeObj);
	CollisionBodyManager manager;
	const t_collision_mesh_id cubeId = manager.loadCollisionModel(cubeFile.path);
	const CollisionModel &cube = manager.getCollisionModel(cubeId);

	// Sphere starting at (-0.2, 0, 0) moving to (0.2, 0, 0) inside cube
	const std::optional<CollisionHit> hit = sweepSphereAgainstMesh(
		cube,
		stationaryIdentity,
		Vector3{-0.2f, 0.0f, 0.0f},
		Vector3{0.4f, 0.0f, 0.0f},
		0.3f,
		standardFrame
	);

	REQUIRE(hit.has_value());
	REQUIRE(hit->collisionDt == Catch::Approx(0.0f));
}

TEST_CASE("Collision at edge: sphere grazing triangle face tangent", "[integration][collision][mesh]")
{
	TemporaryMeshFile cubeFile("unit_test_cube_face_tangent.obj", unitCubeObj);
	CollisionBodyManager manager;
	const t_collision_mesh_id cubeId = manager.loadCollisionModel(cubeFile.path);
	const CollisionModel &cube = manager.getCollisionModel(cubeId);

	// Cube extends from x = -1 to +1. Front face is at z = 1.
	// Sphere radius = 0.5. At z = 1.5, distance to face is exactly 0.5 (tangent).
	// Moving along x from -2 to +2 at z = 1.5.
	const std::optional<CollisionHit> grazingHit = sweepSphereAgainstMesh(
		cube,
		stationaryIdentity,
		Vector3{-2.0f, 0.0f, 1.5f},
		Vector3{4.0f, 0.0f, 0.0f},
		0.5f,
		standardFrame
	);

	REQUIRE(grazingHit.has_value());
	// At x = -1 (cube edge), sphere center reaches x = -1 at t = 0.25
	REQUIRE(grazingHit->collisionDt >= 0.0f);
	REQUIRE(grazingHit->collisionDt <= 1.0f);

	// Now sphere is at z = 1.52 (just 0.02 outside radius 0.5) -> must MISS
	const std::optional<CollisionHit> nearMiss = sweepSphereAgainstMesh(
		cube,
		stationaryIdentity,
		Vector3{-2.0f, 0.0f, 1.52f},
		Vector3{4.0f, 0.0f, 0.0f},
		0.5f,
		standardFrame
	);

	REQUIRE_FALSE(nearMiss.has_value());
}

TEST_CASE("Collision at edge: sphere hitting exact cube edge (shared between two faces)", "[integration][collision][mesh]")
{
	TemporaryMeshFile cubeFile("unit_test_cube_edge_hit.obj", unitCubeObj);
	CollisionBodyManager manager;
	const t_collision_mesh_id cubeId = manager.loadCollisionModel(cubeFile.path);
	const CollisionModel &cube = manager.getCollisionModel(cubeId);

	// Edge along x = 1, y = 1 (top-right edge, parallel to z-axis).
	// Aim sphere towards edge from (2, 2, 0) moving towards (0, 0, 0).
	// Direction is (-1, -1, 0) normalized * distance.
	const float sphereRadius = 0.25f;
	const Vector3 startPos = {2.0f, 2.0f, 0.0f};
	const Vector3 displacement = {-2.0f, -2.0f, 0.0f};

	const std::optional<CollisionHit> hit = sweepSphereAgainstMesh(
		cube,
		stationaryIdentity,
		startPos,
		displacement,
		sphereRadius,
		standardFrame
	);

	REQUIRE(hit.has_value());
	REQUIRE(hit->collisionDt == Catch::Approx((std::sqrt(2.0f) - sphereRadius) / std::sqrt(8.0f)).margin(0.01f));
	REQUIRE(hit->contactNormal.x > 0.0f);
	REQUIRE(hit->contactNormal.y > 0.0f);
}

TEST_CASE("Collision at edge: sphere hitting exact vertex (corner)", "[integration][collision][mesh]")
{
	TemporaryMeshFile cubeFile("unit_test_cube_vertex_hit.obj", unitCubeObj);
	CollisionBodyManager manager;
	const t_collision_mesh_id cubeId = manager.loadCollisionModel(cubeFile.path);
	const CollisionModel &cube = manager.getCollisionModel(cubeId);

	// Corner is at (1, 1, 1).
	// Sphere starts at (2, 2, 2) moving towards (-2, -2, -2).
	const float sphereRadius = 0.2f;
	const Vector3 startPos = {2.0f, 2.0f, 2.0f};
	const Vector3 displacement = {-2.0f, -2.0f, -2.0f};

	const std::optional<CollisionHit> hit = sweepSphereAgainstMesh(
		cube,
		stationaryIdentity,
		startPos,
		displacement,
		sphereRadius,
		standardFrame
	);

	REQUIRE(hit.has_value());
	// The three orthogonal faces meet at corner (1, 1, 1). Along the diagonal from (2, 2, 2)
	// towards (0, 0, 0), the sphere touches the corner at x = y = z = 1 + r = 1.2,
	// which corresponds to t = (2 - (1 + r)) / 2 = (1 - r) / 2 = 0.4f.
	const float expectedDt = (1.0f - sphereRadius) / 2.0f;
	REQUIRE(hit->collisionDt == Catch::Approx(expectedDt).margin(0.01f));
}

TEST_CASE("Collision at edge: high-velocity sweep across thin geometry does not tunnel", "[integration][collision][mesh]")
{
	TemporaryMeshFile cubeFile("unit_test_cube_fast_sweep.obj", unitCubeObj);
	CollisionBodyManager manager;
	const t_collision_mesh_id cubeId = manager.loadCollisionModel(cubeFile.path);
	const CollisionModel &cube = manager.getCollisionModel(cubeId);

	// Sphere radius 0.1, starting at (0, 0, 50.0) moving to (0, 0, -50.0) at 100 units/frame
	const std::optional<CollisionHit> hit = sweepSphereAgainstMesh(
		cube,
		stationaryIdentity,
		Vector3{0.0f, 0.0f, 50.0f},
		Vector3{0.0f, 0.0f, -100.0f},
		0.1f,
		standardFrame
	);

	REQUIRE(hit.has_value());
	REQUIRE(hit->collisionDt == Catch::Approx(0.489f).margin(0.01f));
	REQUIRE(hit->contactNormal.z == Catch::Approx(1.0f).margin(0.01f));
}

TEST_CASE("Collision with scaled and rotated mesh instance", "[integration][collision][mesh]")
{
	TemporaryMeshFile cubeFile("unit_test_cube_transformed.obj", unitCubeObj);
	CollisionBodyManager manager;
	const t_collision_mesh_id cubeId = manager.loadCollisionModel(cubeFile.path);
	const CollisionModel &cube = manager.getCollisionModel(cubeId);

	// Scale 2x, rotated 90 degrees around Y axis
	const Quaternion rotY90 = QuaternionFromAxisAngle(Vector3{0.0f, 1.0f, 0.0f}, PI * 0.5f);
	const CollisionMeshInstance transformedInstance{
		Vector3Zeros,
		Vector3Zeros,
		Vector3Zeros,
		Vector3{2.0f, 2.0f, 2.0f},
		rotY90
	};

	const std::optional<CollisionHit> hit = sweepSphereAgainstMesh(
		cube,
		transformedInstance,
		Vector3{0.0f, 0.0f, 5.0f},
		Vector3{0.0f, 0.0f, -5.0f},
		0.5f,
		standardFrame
	);

	REQUIRE(hit.has_value());
	REQUIRE(hit->collisionDt == Catch::Approx(0.5f).margin(0.02f));
}
