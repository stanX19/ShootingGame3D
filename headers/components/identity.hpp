#ifndef COMPONENTS_IDENTITY_HPP
#define COMPONENTS_IDENTITY_HPP

#include <string>

namespace identity {

struct Name {
	std::string value;
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

#endif // COMPONENTS_IDENTITY_HPP
