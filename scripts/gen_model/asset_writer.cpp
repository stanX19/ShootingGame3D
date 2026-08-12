#include "asset_writer.hpp"
#include "spaceship_topology.hpp"

#include <fstream>
#include <iomanip>
#include <raylib.h>
#include <stdexcept>

namespace gen_model {
	namespace {
		void writeObj(
			const gen_types::MeshData& mesh,
			const std::filesystem::path& path,
			const std::string& modelName,
			bool flipTextureV
		) {
			std::ofstream output(path);
			if (!output) {
				throw std::runtime_error("Unable to write OBJ: " + path.string());
			}
			output << std::fixed << std::setprecision(8);
			output << "mtllib " << modelName << ".mtl\n";
			output << "o " << modelName << "\n";
			for (const auto& point : mesh.positions) {
				output << "v " << point.x << ' ' << point.y << ' ' << point.z << "\n";
			}
			for (const auto& texcoord : mesh.texcoords) {
				const float serializedV = flipTextureV ? 1.0f - texcoord.y : texcoord.y;
				output << "vt " << texcoord.x << ' ' << serializedV << "\n";
			}
			for (const auto& normal : mesh.normals) {
				output << "vn " << normal.x << ' ' << normal.y << ' ' << normal.z << "\n";
			}
			output << "usemtl " << modelName << "\n";
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

		void writeMtl(const std::filesystem::path& path, const std::string& modelName) {
			std::ofstream output(path);
			if (!output) {
				throw std::runtime_error("Unable to write MTL: " + path.string());
			}
			output << "newmtl " << modelName << "\n"
					<< "Ka 0.060000 0.070000 0.080000\n"
					<< "Kd 0.920000 0.940000 0.960000\n"
					<< "Ks 0.620000 0.660000 0.720000\n"
					<< "Ns 48.000000\n"
					<< "map_Kd " << modelName << ".png\n"
					<< "bump " << modelName << "_normal.png\n";
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

		void writeModelAssets(
			const gen_types::AssetData& asset,
			const std::filesystem::path& outputDirectory,
			const std::string& basename,
			bool flipTextureV
		) {
			if (asset.mesh.positions.empty() || asset.mesh.triangles.empty() || asset.texture.rgba.empty() || asset.normalMap.rgba.empty()) {
				throw std::invalid_argument("Cannot write an empty model asset");
			}
			std::filesystem::create_directories(outputDirectory);
			if (basename.empty()) {
				throw std::invalid_argument("Model asset basename cannot be empty");
			}
			writeObj(asset.mesh, outputDirectory / (basename + ".obj"), basename, flipTextureV);
			writeMtl(outputDirectory / (basename + ".mtl"), basename);
			writePng(asset.texture, outputDirectory / (basename + ".png"));
			writePng(asset.normalMap, outputDirectory / (basename + "_normal.png"));
		}
	}

	void writeModelAssets(const gen_types::AssetData& asset, const std::filesystem::path& outputDirectory, const std::string& basename) {
		writeModelAssets(asset, outputDirectory, basename, false);
	}

	void writeSpaceshipAssets(const gen_types::AssetData& asset, const std::filesystem::path& outputDirectory, const std::string& basename) {
		spaceship::topology::requireClosedOrientedMesh(asset.mesh);
		writeModelAssets(asset, outputDirectory, basename, true);
	}

	void writeAsteroidAssets(const gen_types::AssetData& asset, const std::filesystem::path& outputDirectory) {
		writeModelAssets(asset, outputDirectory, "generated_asteroid");
	}

	void writeAsteroidAssets(const gen_types::AssetData& asset, const std::filesystem::path& outputDirectory, const std::string& basename) {
		writeModelAssets(asset, outputDirectory, basename);
	}
} // namespace gen_model
