#include "systems.hpp"
#include "utils.hpp"
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
	entt::entity findClosestTargetInCone(
		TargetsVector targets,
		Vector3 point,
		Vector3 forward,
		float maxAngle,  // 0 to 360 degrees
		float maxDist,  // can be MAXFLOAT
		float angleWeight = 0.7f,
		float distWeight = 0.3f,
		float falloffDist = 50.0f      // distance falloff for scoring
	) {
		entt::entity bestTarget = entt::null;
		float bestScore = -1.0f / 0.0f;

		float cosLimit = cosf(DEG2RAD * maxAngle * 0.5f);

		for (auto [entity, pos] : targets) {
			Vector3 toTarget = pos - point;
			float dist = Vector3Length(toTarget);
			if (dist > maxDist)
				continue;

			Vector3 dir = Vector3Normalize(toTarget);
			float dot = Vector3DotProduct(forward, dir);
			if (dot < cosLimit)
				continue; // outside cone

			// distance factor decays smoothly regardless of maxDist
			float distFactor = 1.0f / (1.0f + dist / falloffDist);

			// weighted combination
			float score = angleWeight * dot + distWeight * distFactor;

			if (score > bestScore) {
				bestScore = score;
				bestTarget = entity;
			}
		}

		return bestTarget;
	}

}

void ecs_systems::aiFindTarget(GameContext &context, [[maybe_unused]] float dt){
	auto targetsView = context.registry.view<faction::Faction, Position, tag::Targetable>();
	std::map<faction::FacVal, TargetsVector> cache;

	auto moveView = context.registry.view<faction::Faction, Position, Rotation, MoveTarget>();
	for (auto [entity, faction, position, rotation, aimTarget] : moveView.each())
	{
		if (cache.find(faction.value) == cache.end())
			cache[faction.value] = searchTargets(targetsView.each(), faction.value);

		aimTarget.entity = findClosestTargetInCone(
			cache[faction.value], position.value,
			getForwardVector(rotation.value),
			360.0f, MAXFLOAT, 0.5f, 0.5f
		);
	}
	
	auto aimView = context.registry.view<faction::Faction, Position, Rotation, AimTarget,
												tag::weapon::IsWeapon, tag::weapon::AIControlledAim>();
	for (auto [entity, faction, position, rotation, aimTarget] : aimView.each())
	{
		if (cache.find(faction.value) == cache.end())
			cache[faction.value] = searchTargets(targetsView.each(), faction.value);

		aimTarget.entity = findClosestTargetInCone(
			cache[faction.value], position.value,
			getForwardVector(rotation.value),
			90.0f, context.config.COMBAT_DIST
		);
	}
}