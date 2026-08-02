#include "catch2/catch_amalgamated.hpp"

#include "collision_body_manager.hpp"
#include "model_manager.hpp"

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
}

TEST_CASE("CollisionBodyManager loads and caches an explicit collision mesh", "[collision-manager]")
{
	TemporaryObj collisionObj("shooting_game_collision_square.obj", squareObj);
	CollisionBodyManager manager;

	t_collision_mesh_id firstID = manager.loadCollisionModel(collisionObj.path);
	t_collision_mesh_id secondID = manager.loadCollisionModel(collisionObj.path);

	REQUIRE(firstID == secondID);
	const CollisionModel &model = manager.getCollisionModel(firstID);
	REQUIRE(model.triangles.size() == 2);
	REQUIRE_FALSE(model.bvh.empty());
	REQUIRE(model.boundingRadius == Catch::Approx(std::sqrt(2.0f)));
}

TEST_CASE("ModelManager rejects invalid path metadata IDs", "[collision-manager]")
{
	ModelManager manager;

	REQUIRE_THROWS_AS(manager.getModelPath(0), std::out_of_range);
}

TEST_CASE("Generated asteroid collision proxy is runtime-loadable", "[collision-manager]")
{
	CollisionBodyManager manager;
	t_collision_mesh_id modelID = manager.loadCollisionModel(
		"assets/Models/asteroid/asteroid_big.collision.obj"
	);
	const CollisionModel &model = manager.getCollisionModel(modelID);

	REQUIRE(model.triangles.size() > 0);
	REQUIRE(model.boundingRadius == Catch::Approx(1.0f));
	REQUIRE_FALSE(model.bvh.empty());
}
