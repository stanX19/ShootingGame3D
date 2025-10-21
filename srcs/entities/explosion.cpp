#include "entities.hpp"
#include "utils.hpp"

void spawnExplosion(GameContext &context, const Vector3& pos, float rad, Vector3 velocity, entt::entity parent) {
	spawnExplosion(context, pos, rad, velocity, 5.0f, ORANGE, parent);
}

void spawnExplosion(GameContext &context, const Vector3& pos, float rad, Vector3 velocity, float lifespan, Color color, entt::entity parent) {
	t_model_id explosionModel = context.modelManager.createSphere(64, 64);
	const float startRatio = 0.05f;

	for (int i = 0; i < 5 * rad; i++)
	{
		entt::entity explosion = context.registry.create();

		Vector3 displaceDir = (i == 0) ? Vector3Zeros: randomUnitVector3();
		float subRad = (i == 0) ? rad : rad / GetRandomValue(2, 5);
		Vector3 subPos = (i == 0) ? pos : pos + displaceDir * rad * startRatio;
		float subLifespan = (i == 0) ? lifespan : lifespan * GetRandomValue(95, 99) / 100.0f;
		float expansion = subRad / subLifespan;
		
		context.registry.emplace<Position>(explosion, subPos);
		context.registry.emplace<Velocity>(explosion, velocity + displaceDir * rad / lifespan);
		context.registry.emplace<RenderBody>(explosion,
			RenderBody{explosionModel, ColorAlpha(color, GetRandomValue(25, 75) / 100.0f), subRad * startRatio}
		);
		context.registry.emplace<RadiusExpand>(explosion, expansion);
		context.registry.emplace<Lifespan>(explosion, subLifespan);

		if (i != 0)
			continue;

		context.registry.emplace<CollisionBody>(explosion, subRad * startRatio);
		context.registry.emplace<Damage>(explosion, 50.0f);
		context.registry.emplace<tag::bullet_type::Energy>(explosion);
		if (context.registry.valid(parent))
			context.registry.emplace<ScoreParent>(explosion, parent);
	}
}

