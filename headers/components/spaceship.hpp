#ifndef COMPONENTS_SPACESHIP_HPP
#define COMPONENTS_SPACESHIP_HPP

#include "includes.hpp"

namespace spaceship {

struct MaxSpeed {
	float value;
};

struct TurnSpeed {
	float value;
};

struct TargetVelocity {
	Vector3 value = { 0, 0, 0 };
	float lerpSpeed = 30.0f;
};

struct TargetRotation {
	Quaternion value = QuaternionIdentity();
	float slerpSpeed = 30.0f;
};

struct MoveTarget {
	entt::entity entity = entt::null;
};

namespace tag {

struct AIMoveControl {};
struct Suicidal {};

} // namespace tag

} // namespace spaceship

namespace tag {
using ::spaceship::tag::AIMoveControl;
using ::spaceship::tag::Suicidal;
} // namespace tag

using ::spaceship::MaxSpeed;
using ::spaceship::TurnSpeed;
using ::spaceship::TargetVelocity;
using ::spaceship::TargetRotation;
using ::spaceship::MoveTarget;

#endif // COMPONENTS_SPACESHIP_HPP
