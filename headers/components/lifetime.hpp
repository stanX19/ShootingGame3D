#ifndef COMPONENTS_LIFETIME_HPP
#define COMPONENTS_LIFETIME_HPP

#include "includes.hpp"

namespace lifetime {

struct Lifespan
{
	float value;
};

struct DisappearBound
{
	Vector3 start;
	Vector3 end;
};

} // namespace lifetime

using ::lifetime::Lifespan;
using ::lifetime::DisappearBound;

#endif // COMPONENTS_LIFETIME_HPP
