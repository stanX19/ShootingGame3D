#include "collision_writer.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

namespace {
	void validateIndex(const int index, const std::size_t count, const char* field) {
		if (index < -1 || index >= static_cast<int>(count))
			throw std::runtime_error(std::string("Invalid collision ") + field + " index");
	}

	void writeFaceVertex(
		std::ostream& output,
		const gen_model::gen_types::Triangle& triangle,
		const std::size_t corner
	) {
		output << triangle.positionIndices[corner] + 1;
		const int texcoord = triangle.texcoordIndices[corner];
		const int normal = triangle.normalIndices[corner];
		if (texcoord < 0 && normal < 0)
			return;
		output << '/';
		if (texcoord >= 0)
			output << texcoord + 1;
		if (normal >= 0)
			output << '/' << normal + 1;
	}
}

void gen_model::collision::writeObj(
	const gen_model::gen_types::MeshData& mesh,
	const std::filesystem::path& path
) {
	if (mesh.positions.empty() || mesh.triangles.empty())
		throw std::runtime_error("Cannot write empty collision geometry");

	for (const auto& triangle : mesh.triangles) {
		for (std::size_t corner = 0; corner < triangle.positionIndices.size(); ++corner) {
			validateIndex(triangle.positionIndices[corner], mesh.positions.size(), "position");
			validateIndex(triangle.texcoordIndices[corner], mesh.texcoords.size(), "texcoord");
			validateIndex(triangle.normalIndices[corner], mesh.normals.size(), "normal");
		}
	}

	std::ofstream output(path);
	if (!output)
		throw std::runtime_error("Cannot open collision output: " + path.string());

	for (const auto& point : mesh.positions)
		output << "v " << point.x << ' ' << point.y << ' ' << point.z << '\n';
	for (const auto& texcoord : mesh.texcoords)
		output << "vt " << texcoord.x << ' ' << texcoord.y << '\n';
	for (const auto& normal : mesh.normals)
		output << "vn " << normal.x << ' ' << normal.y << ' ' << normal.z << '\n';
	for (const auto& triangle : mesh.triangles) {
		output << "f ";
		writeFaceVertex(output, triangle, 0);
		output << ' ';
		writeFaceVertex(output, triangle, 1);
		output << ' ';
		writeFaceVertex(output, triangle, 2);
		output << '\n';
	}

	if (!output)
		throw std::runtime_error("Failed while writing collision output: " + path.string());
}
