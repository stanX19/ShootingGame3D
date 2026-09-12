#include "systems.hpp"
#include "game_context.hpp"
#include "game_utils.hpp"
#include "entities.hpp"
#include "components/physics.hpp"
#include "components/render.hpp"
#include "components/identity.hpp"
#include "components/faction.hpp"
#include "utils/color_utils.hpp"
#include <array>

namespace {

using SpawnFn = entt::entity(*)(GameContext&, const Vector3&, faction::Faction);

const std::array<SpawnFn, 8> kShipSpawners = {
	spawnUnit,
	spawnFighterUnit,
	spawnEliteUnit,
	spawnFastEliteUnit,
	spawnTerminatorUnit,
	spawnMothershipUnit,
	spawnInterceptorQuadUnit,
	spawnHeavyQuadUnit
};

Vector3 generateSpawnPos(const GameContext& context, const Vector3& playerPos, float zSign) {
	const float halfArena = context.config.ARENA_SIZE * 0.5f;

	// Ratio of half-arena: wide XY spread across spawn
	const float minXRatio = -0.9f;
	const float maxXRatio = +0.9f;
	const float minYRatio = -0.9f;
	const float maxYRatio = +0.9f;

	// Symmetrical Z distance range from center (0.0 = center, 1.0 = arena edge)
	const float minZDistRatio = 0.5f;
	const float maxZDistRatio = 0.9f;

	const float zA = zSign * (minZDistRatio * halfArena);
	const float zB = zSign * (maxZDistRatio * halfArena);

	const float startZ = std::min(zA, zB);
	const float endZ = std::max(zA, zB);

	return game_utils::randomPosInBoxOffCombat(
		Vector3{minXRatio * halfArena, minYRatio * halfArena, startZ},
		Vector3{maxXRatio * halfArena, maxYRatio * halfArena, endZ},
		playerPos,
		context.config.COMBAT_DIST
	);
}

void applyFactionTags(GameContext& context, entt::entity entity, faction::Faction faction, Color bodyColor) {
	context.registry.emplace_or_replace<faction::Faction>(entity, faction);
	render::RenderBody* body = context.registry.try_get<render::RenderBody>(entity);
	if (body) {
		body->color = bodyColor;
	}
}

void spawnFactionUnits(GameContext& context, faction::Faction faction, float zSign, Color bodyColor) {
	Vector3 playerPos = {0, 0, 0};
	if (context.registry.valid(context.currentPlayer)) {
		playerPos = context.registry.get<physics::Position>(context.currentPlayer).value;
	}

	auto view = context.registry.view<faction::Faction, identity::tag::Spaceship>();
	int factionCount = 0;
	for (auto entity : view) {
		if (view.get<faction::Faction>(entity).value == faction.value) {
			factionCount++;
		}
	}

	int toSpawn = context.config.UNIT_COUNT - factionCount;
	for (int i = 0; i < toSpawn; ++i) {
		Vector3 spawnPos = generateSpawnPos(context, playerPos, zSign);
		int spawnerIdx = GetRandomValue(0, (int)kShipSpawners.size() - 1);
		entt::entity unit = kShipSpawners[spawnerIdx](context, spawnPos, faction);
		applyFactionTags(context, unit, faction, bodyColor);
	}
}

} // namespace

void systems::UnitSpawn::update(GameContext& context, [[maybe_unused]] float dt) {
	spawnFactionUnits(context, {faction::FAC_BLUE}, -1.0f, ColorLerp(WHITE, SKYBLUE, 0.1f));
	spawnFactionUnits(context, {faction::FAC_RED}, +1.0f, WHITE);
}
