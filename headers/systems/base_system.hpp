#pragma once

struct GameContext;

namespace systems {

class BaseSystem {
public:
	virtual ~BaseSystem() = default;
	virtual void update(GameContext &context, float dt) = 0;
};

} // namespace systems

using BaseSystem = systems::BaseSystem;

