#include "systems.hpp"

void ecs_systems::cleanOutOfBound(GameContext &context)
{
	auto view = context.registry.view<DisappearBound, Position>();
	std::vector<entt::entity> toDestroy;

	for (auto entity : view)
	{
		DisappearBound &bound = view.get<DisappearBound>(entity);
		Vector3 &pos = view.get<Position>(entity).value;
		Vector3 &start = bound.start;
		Vector3 &end = bound.end;

		bool outOfBounds =
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
