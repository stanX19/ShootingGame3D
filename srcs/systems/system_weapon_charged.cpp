#include "systems.hpp"
#include "utils.hpp"
#include <iostream>

namespace {
	entt::entity createChargeEffectEntity(
		GameContext &context,
		entt::entity weaponEntity,
		const Vector3 &position,
		float chargeRatio,
		float remainingTime,
		Color color
	) {
		entt::entity chargeEffectEntity = context.registry.create();

		float chargeRadius = 0.5f * (1 - chargeRatio); // linear scale for now

		context.registry.emplace<RenderBody>(chargeEffectEntity, RenderBody{
			context.modelManager.createSphere(),
			color,
			chargeRadius
		});
		context.registry.emplace<Lifespan>(chargeEffectEntity, Lifespan{remainingTime});
		context.registry.emplace<RadiusExpand>(chargeEffectEntity, RadiusExpand{-chargeRadius / remainingTime});
		context.registry.emplace<Position>(chargeEffectEntity, position);
		context.registry.emplace<PositionAnchor>(chargeEffectEntity, weaponEntity, Vector3{0.0f, 0.0f, 0.0f});
		context.registry.emplace<DeathAnchor>(chargeEffectEntity, weaponEntity, 0.25f);

		return chargeEffectEntity;
	}
}

void ecs_systems::weaponUpdateCharged(GameContext &context, [[maybe_unused]] float dt)
{
	auto view = context.registry.view<Weapon, Position, ChargedWeapon>();

	for (auto [entity, weapon, position, chargedWeapon] : view.each())
	{
		bool isCharging = chargedWeapon.currentCharge && chargedWeapon.currentCharge < chargedWeapon.totalChargeNeeded;
		if (isCharging && chargedWeapon.chargeEffectEntity == entt::null) {
			chargedWeapon.chargeEffectEntity = createChargeEffectEntity(
				context,
				entity,
				position.value,
				chargedWeapon.currentCharge / chargedWeapon.totalChargeNeeded,
				chargedWeapon.totalChargeNeeded - chargedWeapon.currentCharge,
				chargedWeapon.effectColor
			);
		} else if (!isCharging && chargedWeapon.chargeEffectEntity != entt::null) {
			if (context.registry.valid(chargedWeapon.chargeEffectEntity))
				context.registry.destroy(chargedWeapon.chargeEffectEntity);
			chargedWeapon.chargeEffectEntity = entt::null;
		}
	}
}
