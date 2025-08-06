#include "entities.hpp"
#include "utils.hpp"

void spawnExplosion(GameContext &context, const Vector3& pos, float rad, Vector3 velocity, float lifespan, Color color) {
	t_model_id explosionModel = context.meshManager.createSphere();

	for (int i = 0; i < 5 * rad; i++)
	{
		entt::entity explosion = context.registry.create();

		float expansion = 10.0f;
		Vector3 displaceDir = (i == 0) ? Vector3Zeros: randomUnitVector3();
		float subRad = (i == 0) ? rad : GetRandomValue(rad / 5, rad / 2);
		Vector3 subPos = (i == 0) ? pos : pos + displaceDir * (rad + subRad);
		
		context.registry.emplace<Position>(explosion, subPos);
		context.registry.emplace<Velocity>(explosion, velocity + displaceDir * rad * expansion);
		context.registry.emplace<RenderBody>(explosion,
			RenderBody{explosionModel, ColorAlpha(color, GetRandomValue(25, 75) / 100.0f), subRad}
		);
		context.registry.emplace<RadiusExpand>(explosion, expansion);
		context.registry.emplace<Lifespan>(explosion, lifespan + GetRandomValue(0, 200) / 100.0f);
	}
}

