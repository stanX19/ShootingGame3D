#include "systems.hpp"
#include "game_context.hpp"
#include "components/lifetime.hpp"
#include "components/physics.hpp"
#include <vector>

void systems::CleanOutOfBound::update(GameContext &context, [[maybe_unused]] float dt)
{
	auto view = context.registry.view<const DisappearBound, const Position>();
	std::vector<entt::entity> toDestroy;

	for (auto entity : view)
	{
		const DisappearBound &bound = view.get<const DisappearBound>(entity);
		const Vector3 &pos = view.get<const Position>(entity).value;
		const Vector3 &start = bound.start;
		const Vector3 &end = bound.end;

		const bool outOfBounds =
			(pos.x < start.x || pos.x > end.x) ||
			(pos.y < start.y || pos.y > end.y) ||
			(pos.z < start.z || pos.z > end.z);

		if (outOfBounds)
		{
			toDestroy.push_back(entity);
		}
	}

	for (auto entity : toDestroy)
	{
		if (context.registry.valid(entity))
		{
			context.registry.destroy(entity);
		}
	}
}

