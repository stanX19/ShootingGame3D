#pragma once

#include "gen_model/gen_types.hpp"

#include <filesystem>

namespace gen_model::collision {
	gen_model::gen_types::MeshData importObj(const std::filesystem::path& path);
}
