#include "catch2/catch_amalgamated.hpp"

#include "gen_model/collision/collision_writer.hpp"
#include "gen_model/collision/model_importer.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace {
	std::filesystem::path fixturePath(const std::string& name) {
		return std::filesystem::temp_directory_path() / name;
	}

	void writeFixture(const std::filesystem::path& path, const std::string& contents) {
		std::ofstream output(path);
		REQUIRE(output);
		output << contents;
	}
}

TEST_CASE("collision OBJ importer preserves shared gen_model geometry types", "[unit][collision]")
{
	const auto input = fixturePath("collision_passthrough_import.obj");
	writeFixture(input,
		"mtllib ignored.mtl\n"
		"o triangle\n"
		"v 0 0 0\n"
		"v 1 0 0\n"
		"v 0 1 0\n"
		"vt 0 0\n"
		"vt 1 0\n"
		"vt 0 1\n"
		"vn 0 0 1\n"
		"f 1/1/1 2/2/1 3/3/1\n");

	const auto mesh = gen_model::collision::importObj(input);

	REQUIRE(mesh.positions == std::vector<gen_model::gen_types::Point3>{
		{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}
	});
	REQUIRE(mesh.texcoords.size() == 3);
	REQUIRE(mesh.normals.size() == 1);
	REQUIRE(mesh.triangles.size() == 1);
	REQUIRE(mesh.triangles[0].positionIndices == std::array<int, 3>{0, 1, 2});
	REQUIRE(mesh.triangles[0].texcoordIndices == std::array<int, 3>{0, 1, 2});
	REQUIRE(mesh.triangles[0].normalIndices == std::array<int, 3>{0, 0, 0});

	std::filesystem::remove(input);
}

TEST_CASE("collision OBJ round trip preserves optional attributes", "[unit][collision]")
{
	const auto input = fixturePath("collision_passthrough_optional.obj");
	const auto output = fixturePath("collision_passthrough_optional.collision.obj");
	writeFixture(input,
		"v 0 0 0\n"
		"v 1 0 0\n"
		"v 0 1 0\n"
		"vn 0 0 1\n"
		"f 1//1 2//1 3//1\n");

	const auto original = gen_model::collision::importObj(input);
	gen_model::collision::writeObj(original, output);
	const auto roundTrip = gen_model::collision::importObj(output);

	REQUIRE(roundTrip.positions == original.positions);
	REQUIRE(roundTrip.texcoords == original.texcoords);
	REQUIRE(roundTrip.normals == original.normals);
	REQUIRE(roundTrip.triangles == original.triangles);
	REQUIRE(roundTrip.triangles[0].texcoordIndices == std::array<int, 3>{-1, -1, -1});
	REQUIRE(roundTrip.triangles[0].normalIndices == std::array<int, 3>{0, 0, 0});

	std::ifstream written(output);
	const std::string contents{
		std::istreambuf_iterator<char>(written),
		std::istreambuf_iterator<char>()
	};
	REQUIRE(contents.find("mtllib") == std::string::npos);
	REQUIRE(contents.find("usemtl") == std::string::npos);

	std::filesystem::remove(input);
	std::filesystem::remove(output);
}

TEST_CASE("collision OBJ importer resolves negative indices", "[unit][collision]")
{
	const auto input = fixturePath("collision_passthrough_negative.obj");
	writeFixture(input,
		"v 0 0 0\n"
		"v 1 0 0\n"
		"v 0 1 0\n"
		"f -3 -2 -1\n");

	const auto mesh = gen_model::collision::importObj(input);

	REQUIRE(mesh.triangles.size() == 1);
	REQUIRE(mesh.triangles[0].positionIndices == std::array<int, 3>{0, 1, 2});
	REQUIRE(mesh.triangles[0].texcoordIndices == std::array<int, 3>{-1, -1, -1});
	REQUIRE(mesh.triangles[0].normalIndices == std::array<int, 3>{-1, -1, -1});

	std::filesystem::remove(input);
}

TEST_CASE("collision OBJ importer rejects malformed or empty geometry", "[unit][collision]")
{
	const auto empty = fixturePath("collision_passthrough_empty.obj");
	const auto malformed = fixturePath("collision_passthrough_malformed.obj");
	writeFixture(empty, "# no geometry\n");
	writeFixture(malformed, "v 0 0\n");

	REQUIRE_THROWS(gen_model::collision::importObj(empty));
	REQUIRE_THROWS(gen_model::collision::importObj(malformed));

	std::filesystem::remove(empty);
	std::filesystem::remove(malformed);
}
