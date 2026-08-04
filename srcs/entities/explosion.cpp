#include "entities.hpp"
#include "utils.hpp"
#include <algorithm>

namespace
{
	constexpr float MIN_RENDER_RADIUS = 0.0001f;

	void inheritExplosionParent(GameContext &context, entt::entity entity, entt::entity parent)
	{
		if (!context.registry.valid(parent))
			return;

		context.registry.emplace<ScoreParent>(entity, parent);
		context.registry.emplace<faction::Faction>(
			entity, context.registry.get_or_emplace<faction::Faction>(parent).value
		);
	}

	void spawnExplosionInternal(
		GameContext &context,
		const Vector3 &pos,
		float startRadius,
		float finalRadius,
		float lifespan,
		float damage,
		Color color,
		Vector3 velocity,
		entt::entity parent
	)
	{
		if (startRadius < 0.0f || finalRadius <= startRadius || lifespan <= 0.0f)
			return;

		t_model_id explosionModel = context.modelManager.createSphere(64, 64);
		const int fragmentCount = std::max(1, static_cast<int>(5.0f * finalRadius));
		const float startRatio = startRadius / finalRadius;

		for (int i = 0; i < fragmentCount; i++)
		{
			const bool isCore = i == 0;
			Vector3 displaceDir = isCore ? Vector3Zeros : randomUnitVector3();
			float subRad = isCore ? finalRadius : finalRadius / GetRandomValue(2, 5);
			Vector3 subPos = isCore ? pos : pos + displaceDir * startRadius;
			float subLifespan = isCore ? lifespan : lifespan * GetRandomValue(95, 99) / 100.0f;
			float subStartRadius = isCore ? startRadius : subRad * startRatio;
			float expansion = (subRad - subStartRadius) / subLifespan;
			float renderStartRadius = std::min(subRad, std::max(subStartRadius, MIN_RENDER_RADIUS));

			entt::entity explosion = context.registry.create();
			context.registry.emplace<Position>(explosion, subPos);
			context.registry.emplace<Velocity>(explosion, velocity + displaceDir * finalRadius / lifespan);
			context.registry.emplace<RenderBody>(explosion,
				RenderBody{explosionModel, ColorAlpha(color, GetRandomValue(25, 75) / 100.0f), renderStartRadius}
			);
			context.registry.emplace<RadiusExpand>(explosion, expansion);
			context.registry.emplace<Lifespan>(explosion, subLifespan);

			if (!isCore || damage <= 0.0f)
				continue;

			context.registry.emplace<CollisionBody>(explosion, subStartRadius);
			context.registry.emplace<Damage>(explosion, damage);
			context.registry.emplace<tag::bullet_type::Energy>(explosion);
			inheritExplosionParent(context, explosion, parent);
		}
	}
}

void spawnExplosion(GameContext &context, const Vector3& pos, float rad, Vector3 velocity, entt::entity parent)
{
	spawnExplosion(context, pos, rad, velocity, effect::DEFAULT_EXPLOSION_DURATION, effect::EXPLOSION_COLOR, parent);
}

void spawnExplosion(GameContext &context, const Vector3& pos, float rad, Vector3 velocity, float lifespan, Color color, entt::entity parent)
{
	spawnExplosionInternal(
		context,
		pos,
		0.0f,
		rad,
		lifespan,
		effect::DEFAULT_EXPLOSION_DAMAGE,
		color,
		velocity,
		parent
	);
}

void spawnExplosion(GameContext &context, const Vector3& pos, const effect::ExplodeOnDeath &effect, Vector3 velocity, entt::entity parent)
{
	spawnExplosionInternal(
		context,
		pos,
		effect.startRadius,
		effect.finalRadius,
		effect.explosionDuration,
		effect.damage,
		effect.color,
		velocity,
		parent
	);
}

void spawnInstantDamage(GameContext &context, const Vector3& pos, const effect::InstantDamageOnDeath &effect, entt::entity parent)
{
	if (effect.instantDamage <= 0.0f || effect.radius <= 0.0f)
		return;

	entt::entity damagePulse = context.registry.create();
	context.registry.emplace<Position>(damagePulse, pos);
	context.registry.emplace<CollisionBody>(damagePulse, effect.radius);
	context.registry.emplace<Damage>(damagePulse, effect.instantDamage);
	context.registry.emplace<Lifespan>(damagePulse, 0.0f);
	context.registry.emplace<tag::bullet_type::Energy>(damagePulse);
	inheritExplosionParent(context, damagePulse, parent);
}
