#include "systems.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "components/factions.hpp"

namespace {
	using TargetsVector = std::vector<std::tuple<entt::entity, Vector3>>;

	template<typename View>
	TargetsVector searchTargets(View &&targetsIter, faction::FacVal myFaction) {
		TargetsVector ret;

		for (auto [entity, targetFaction, position] : targetsIter) {
			if (targetFaction.value & myFaction)
				continue;
			ret.push_back({entity, position.value});
		}

		return ret;
	}

	// angle: cone angle
	// minDist: search distance limit
	entt::entity findClosestTargetInCone(TargetsVector targets, Vector3 point, Vector3 forward, float angle=30.0f, float minDist = COMBAT_DIST) {
		entt::entity target = entt::null;
		float bestDot = cosf(DEG2RAD * angle * 0.5);

		for (auto [entity, pos]: targets) {
			Vector3 toTarget = pos - (point - forward * 2.5f);
			float dist = Vector3Length(toTarget);
			if (dist >= minDist)
				continue;
			float dot = Vector3DotProduct(forward, Vector3Normalize(toTarget));
			if (dot < bestDot)
				continue;
			bestDot = dot;
			target = entity;
		}

		return target;
	}
}

void ecs_systems::aiFindTarget(GameContext &context){
	auto targetsView = context.registry.view<faction::Faction, Position, tag::Targetable>();
	std::map<faction::FacVal, TargetsVector> cache;

	auto moveView = context.registry.view<faction::Faction, Position, Rotation, MoveTarget>();
	for (auto [entity, faction, position, rotation, aimTarget] : moveView.each())
	{
		if (cache.find(faction.value) == cache.end())
			cache[faction.value] = searchTargets(targetsView.each(), faction.value);

		aimTarget.entity = findClosestTargetInCone(cache[faction.value], position.value, getForwardVector(rotation.value), 180.0f, MAXFLOAT);
	}
	
	auto aimView = context.registry.view<faction::Faction, Position, Rotation, AimTarget,
												tag::weapon::IsWeapon, tag::weapon::AIControlledAim>();
	for (auto [entity, faction, position, rotation, aimTarget] : aimView.each())
	{
		if (cache.find(faction.value) == cache.end())
			cache[faction.value] = searchTargets(targetsView.each(), faction.value);

		aimTarget.entity = findClosestTargetInCone(cache[faction.value], position.value, getForwardVector(rotation.value));
	}
}