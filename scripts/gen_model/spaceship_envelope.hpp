#ifndef GEN_MODEL_SPACESHIP_ENVELOPE_HPP
#define GEN_MODEL_SPACESHIP_ENVELOPE_HPP

#include "spaceship_design.hpp"
#include "spaceship_mesh.hpp"

namespace gen_model::spaceship::envelope {
	SystemDetailReport appendFunctionalFairings(
		detail::MeshBuilder& builder,
		const Settings& settings,
		const design::DesignPlan& plan
	);
} // namespace gen_model::spaceship::envelope

#endif // GEN_MODEL_SPACESHIP_ENVELOPE_HPP
