#pragma once

#include "gen_model/gen_types.hpp"

#include <filesystem>

namespace gen_model::collision {
	void writeObj(
		const gen_model::gen_types::MeshData& mesh,
		const std::filesystem::path& path
	);
}
