#include "asset_writer.hpp"

#include <fstream>
#include <iomanip>
#include <raylib.h>
#include <stdexcept>

namespace gen_model {
	namespace {
		constexpr const char* MODEL_NAME = "generated_asteroid";

		void writeObj(const gen_types::MeshData& mesh, const std::filesystem::path& path) {
			std::ofstream output(path);
			if (!output) {
				throw std::runtime_error("Unable to write OBJ: " + path.string());
			}
			output << std::fixed << std::setprecision(8);
			output << "mtllib " << MODEL_NAME << ".mtl\n";
			output << "o " << MODEL_NAME << "\n";
			for (const auto& point : mesh.positions) {
				output << "v " << point.x << ' ' << point.y << ' ' << point.z << "\n";
			}
			for (const auto& texcoord : mesh.texcoords) {
				output << "vt " << texcoord.x << ' ' << texcoord.y << "\n";
			}
			for (const auto& normal : mesh.normals) {
				output << "vn " << normal.x << ' ' << normal.y << ' ' << normal.z << "\n";
			}
			output << "usemtl " << MODEL_NAME << "\n";
			output << "s off\n";
			for (const auto& triangle : mesh.triangles) {
				output << "f";
				for (int corner = 0; corner < 3; ++corner) {
					output << ' ' << triangle.positionIndices[corner] + 1
							<< '/' << triangle.texcoordIndices[corner] + 1
							<< '/' << triangle.normalIndices[corner] + 1;
				}
				output << "\n";
			}
			if (!output) {
				throw std::runtime_error("Unable to finish OBJ: " + path.string());
			}
		}

		void writeMtl(const std::filesystem::path& path) {
			std::ofstream output(path);
			if (!output) {
				throw std::runtime_error("Unable to write MTL: " + path.string());
			}
			output << "newmtl " << MODEL_NAME << "\n"
					<< "Ka 0.200000 0.180000 0.160000\n"
					<< "Kd 1.000000 1.000000 1.000000\n"
					<< "Ks 0.050000 0.050000 0.050000\n"
					<< "Ns 8.000000\n"
					<< "map_Kd " << MODEL_NAME << ".png\n"
					<< "bump " << MODEL_NAME << "_normal.png\n";
			if (!output) {
				throw std::runtime_error("Unable to finish MTL: " + path.string());
			}
		}

		void writePng(const gen_types::TextureData& texture, const std::filesystem::path& path) {
			Image image{
				static_cast<void*>(const_cast<std::uint8_t*>(texture.rgba.data())),
				texture.width,
				texture.height,
				1,
				PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
			};
			if (!ExportImage(image, path.string().c_str())) {
				throw std::runtime_error("Unable to write PNG: " + path.string());
			}
		}
	}

	void writeAsteroidAssets(const gen_types::AssetData& asset, const std::filesystem::path& outputDirectory) {
		if (asset.mesh.positions.empty() || asset.mesh.triangles.empty() || asset.texture.rgba.empty() || asset.normalMap.rgba.empty()) {
			throw std::invalid_argument("Cannot write an empty asteroid asset");
		}
		std::filesystem::create_directories(outputDirectory);
		writeObj(asset.mesh, outputDirectory / (std::string(MODEL_NAME) + ".obj"));
		writeMtl(outputDirectory / (std::string(MODEL_NAME) + ".mtl"));
		writePng(asset.texture, outputDirectory / (std::string(MODEL_NAME) + ".png"));
		writePng(asset.normalMap, outputDirectory / (std::string(MODEL_NAME) + "_normal.png"));
	}
} // namespace gen_model
