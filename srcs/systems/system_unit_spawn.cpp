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

const std::array<SpawnFn, 6> kShipSpawners = {
	spawnUnit,
	spawnFighterUnit,
	spawnEliteUnit,
	spawnFastEliteUnit,
	spawnTerminatorUnit,
	spawnMothershipUnit
};

Vector3 generateSpawnPos(const GameContext& context, const Vector3& playerPos, float zSign) {
	float arenaSize = context.config.ARENA_SIZE;
	const float box = 0.2f;
	const float dist = 0.2f;

	float minZ = (zSign < 0.0f) ? -arenaSize * (1.0f - dist) : arenaSize * (1.0f - box - dist);
	float maxZ = (zSign < 0.0f) ? -arenaSize * (1.0f - box - dist) : arenaSize * (1.0f - dist);

	return game_utils::randomPosInBoxOffCombat(
		Vector3{-arenaSize * box, -arenaSize * box, minZ},
		Vector3{+arenaSize * box, +arenaSize * box, maxZ},
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
