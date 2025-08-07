#include "entities.hpp"
#include "utils.hpp"

void spawnExplosion(GameContext &context, const Vector3& pos, float rad, Vector3 velocity, float lifespan, Color color) {
	t_model_id explosionModel = context.meshManager.createSphere(64, 64);

	for (int i = 0; i < 5 * rad; i++)
	{
		entt::entity explosion = context.registry.create();

		Vector3 displaceDir = (i == 0) ? Vector3Zeros: randomUnitVector3();
		float subRad = (i == 0) ? rad : rad / GetRandomValue(2, 5);
		// Vector3 subPos = (i == 0) ? pos : pos + displaceDir * (rad);
		float subLifespan = (i == 0) ? lifespan : lifespan * GetRandomValue(90, 99) / 100.0f;
		float expansion = subRad / subLifespan;
		
		context.registry.emplace<Position>(explosion, pos);
		context.registry.emplace<Velocity>(explosion, velocity + displaceDir * rad / lifespan);
		context.registry.emplace<RenderBody>(explosion,
			RenderBody{explosionModel, ColorAlpha(color, GetRandomValue(25, 75) / 100.0f), subRad * 0.001f}
		);
		context.registry.emplace<RadiusExpand>(explosion, expansion);
		context.registry.emplace<Lifespan>(explosion, subLifespan);
	}
}

