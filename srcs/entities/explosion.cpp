#include "entities.hpp"
#include "utils.hpp"
#include "components/identity.hpp"
#include <algorithm>

namespace
{
	constexpr float MIN_RENDER_RADIUS = 0.0001f;

	void inheritExplosionParent(GameContext &context, entt::entity entity, entt::entity parent)
	{
		if (!context.registry.valid(parent))
			return;

		const auto *parentOwner = context.registry.try_get<identity::Owner>(parent);
		const entt::entity rootOwner = parentOwner ? parentOwner->root : parent;
		context.registry.emplace<identity::Owner>(entity, rootOwner);
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

		t_model_id explosionModel = context.modelManager.createSphere(10, 10);
		const int fragmentCount = std::clamp(static_cast<int>(2.0f * finalRadius), 4, 32);
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
			context.registry.emplace<physics::Position>(explosion, subPos);
			context.registry.emplace<physics::Velocity>(explosion, velocity + displaceDir * finalRadius / lifespan);
			context.registry.emplace<render::RenderBody>(explosion,
				render::RenderBody{explosionModel, ColorAlpha(color, GetRandomValue(1, 3) * 0.25f), renderStartRadius}
			);
			context.registry.emplace<render::RadiusExpand>(explosion, expansion);
			context.registry.emplace<lifetime::Lifespan>(explosion, subLifespan);

			if (!isCore || damage <= 0.0f)
				continue;

			context.registry.emplace<collision::CollisionBody>(explosion, subStartRadius);
			context.registry.emplace<combat::Damage>(explosion, damage);
			context.registry.emplace<weapon::tag::Energy>(explosion);
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
	context.registry.emplace<physics::Position>(damagePulse, pos);
	context.registry.emplace<collision::CollisionBody>(damagePulse, effect.radius);
	context.registry.emplace<combat::Damage>(damagePulse, effect.instantDamage);
	context.registry.emplace<lifetime::Lifespan>(damagePulse, 0.0f);
	context.registry.emplace<weapon::tag::Energy>(damagePulse);
	inheritExplosionParent(context, damagePulse, parent);
}
