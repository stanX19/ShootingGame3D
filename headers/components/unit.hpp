#pragma once

#include <string>

struct Name {
	std::string val;
};

namespace tag {
	struct Asteroid {};
	struct Missile {};
	struct AIMoveControl {};
	struct Suicidal {};
	struct Enemy {};
	struct EliteUnit {};
	struct Bullet {};
	struct Targetable {};
	struct Spaceship {};
}
