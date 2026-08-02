#include "model_importer.hpp"

#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
	int parseInteger(const std::string& value, const std::string_view field) {
		try {
			std::size_t parsed = 0;
			const int result = std::stoi(value, &parsed);
			if (parsed != value.size())
				throw std::runtime_error("trailing characters");
			return result;
		} catch (const std::exception&) {
			throw std::runtime_error("Invalid OBJ " + std::string(field) + " index");
		}
	}

	int resolveIndex(const int rawIndex, const std::size_t count, const std::string_view field) {
		if (rawIndex == 0)
			throw std::runtime_error("OBJ " + std::string(field) + " index cannot be zero");

		const int resolved = rawIndex > 0
			? rawIndex - 1
			: static_cast<int>(count) + rawIndex;
		if (resolved < 0 || resolved >= static_cast<int>(count))
			throw std::runtime_error("OBJ " + std::string(field) + " index is out of range");
		return resolved;
	}

	gen_model::gen_types::Triangle parseFace(
		const std::array<std::string, 3>& tokens,
		const gen_model::gen_types::MeshData& mesh
	) {
		gen_model::gen_types::Triangle triangle;
		for (std::size_t corner = 0; corner < tokens.size(); ++corner) {
			std::stringstream fields(tokens[corner]);
			std::string field;
			if (!std::getline(fields, field, '/') || field.empty())
				throw std::runtime_error("Malformed OBJ face vertex");
			triangle.positionIndices[corner] = resolveIndex(
				parseInteger(field, "position"), mesh.positions.size(), "position");

			triangle.texcoordIndices[corner] = -1;
			triangle.normalIndices[corner] = -1;
			if (!std::getline(fields, field, '/'))
				continue;
			if (!field.empty())
				triangle.texcoordIndices[corner] = resolveIndex(
					parseInteger(field, "texcoord"), mesh.texcoords.size(), "texcoord");

			if (!std::getline(fields, field, '/'))
				continue;
			if (!field.empty())
				triangle.normalIndices[corner] = resolveIndex(
					parseInteger(field, "normal"), mesh.normals.size(), "normal");

			if (std::getline(fields, field, '/'))
				throw std::runtime_error("Malformed OBJ face vertex");
		}
		return triangle;
	}
}

gen_model::gen_types::MeshData gen_model::collision::importObj(const std::filesystem::path& path) {
	std::ifstream input(path);
	if (!input)
		throw std::runtime_error("Cannot open OBJ input: " + path.string());

	gen_model::gen_types::MeshData mesh;
	std::string line;
	while (std::getline(input, line)) {
		std::istringstream stream(line);
		std::string record;
		stream >> record;
		if (record.empty() || record.front() == '#')
			continue;

		if (record == "v") {
			gen_model::gen_types::Point3 point;
			if (!(stream >> point.x >> point.y >> point.z))
				throw std::runtime_error("Malformed OBJ vertex record");
			mesh.positions.push_back(point);
			continue;
		}
		if (record == "vt") {
			gen_model::gen_types::Point2 texcoord;
			if (!(stream >> texcoord.x >> texcoord.y))
				throw std::runtime_error("Malformed OBJ texcoord record");
			mesh.texcoords.push_back(texcoord);
			continue;
		}
		if (record == "vn") {
			gen_model::gen_types::Point3 normal;
			if (!(stream >> normal.x >> normal.y >> normal.z))
				throw std::runtime_error("Malformed OBJ normal record");
			mesh.normals.push_back(normal);
			continue;
		}
		if (record != "f")
			continue;

		std::array<std::string, 3> tokens;
		for (std::string& token : tokens) {
			if (!(stream >> token))
				throw std::runtime_error("Only triangular OBJ faces are supported");
		}
		std::string extra;
		if (stream >> extra && extra.front() != '#')
			throw std::runtime_error("Only triangular OBJ faces are supported");
		mesh.triangles.push_back(parseFace(tokens, mesh));
	}

	if (mesh.positions.empty() || mesh.triangles.empty())
		throw std::runtime_error("OBJ contains no collision geometry: " + path.string());
	return mesh;
}
