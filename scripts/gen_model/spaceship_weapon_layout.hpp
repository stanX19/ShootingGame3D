#ifndef GEN_MODEL_SPACESHIP_WEAPON_LAYOUT_HPP
#define GEN_MODEL_SPACESHIP_WEAPON_LAYOUT_HPP

#include <string>
#include <vector>

#include "spaceship_design.hpp"

namespace gen_model::spaceship::weapon_layout {
	struct CandidateLayout {
		std::vector<MountSettings> mounts;
		std::vector<design::ModuleVolume> modules;
		float coverageScore = 0.0f;
		float separationScore = 0.0f;
		float structuralScore = 0.0f;
		float integrationScore = 0.0f;
		float balanceScore = 0.0f;
		float totalScore = 0.0f;
		bool operator==(const CandidateLayout&) const = default;
	};

	struct LayoutCandidates {
		std::vector<CandidateLayout> ranked;
		std::vector<std::string> rejectionDiagnostics;

		const CandidateLayout& best() const;
	};

	LayoutCandidates plan(
		const Settings& settings,
		const design::PreliminaryDesign& preliminary
	);

	float unitParentCollisionClearance(const MountSettings& mount);
} // namespace gen_model::spaceship::weapon_layout

#endif // GEN_MODEL_SPACESHIP_WEAPON_LAYOUT_HPP
