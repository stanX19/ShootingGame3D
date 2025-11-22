#ifndef FACTIONS_HPP
#define FACTIONS_HPP

namespace faction {
	using FacVal = unsigned int;

	enum : FacVal {
		FAC_NONE = 0,
		FAC_BLUE = 1 << 0,
		FAC_RED = 1 << 1,
		FAC_BULLET = 1 << 2,
		FAC_ASTEROID = 1 << 3
	};
	
	struct Faction {
		FacVal value = FAC_NONE;
	};
}

#endif