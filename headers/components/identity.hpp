#ifndef COMPONENTS_IDENTITY_HPP
#define COMPONENTS_IDENTITY_HPP

#include <string>
#include "includes.hpp"

namespace identity {

struct Name {
	std::string value;
};

struct Owner {
	entt::entity root = entt::null;
};

namespace tag {

struct Spaceship {};
struct Asteroid {};

} // namespace tag

} // namespace identity

namespace tag {
using ::identity::tag::Spaceship;
using ::identity::tag::Asteroid;
} // namespace tag

using ::identity::Name;
using ::identity::Owner;

#endif // COMPONENTS_IDENTITY_HPP
