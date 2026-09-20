#include "catch2/catch_amalgamated.hpp"

#include "game_context.hpp"
#include "systems.hpp"
#include "events.hpp"
#include "entities/spaceship_factory.hpp"
#include "components/combat.hpp"
#include "components/collision.hpp"
#include "components/physics.hpp"
#include "components/render.hpp"
#include "components/faction.hpp"

#include <vector>

namespace
{
	const std::vector<GameConfig::RootSource> kSmokeConfigRoots{
		{"audio", "assets/config/audio.json"},
		{"debug", "assets/config/debug.json"},
		{"game", "assets/config/game.json"},
		{"hud", "assets/config/hud.json"},
		{"loadout", "assets/config/loadout.json"},
		{"physics", "assets/config/physics.json"},
		{"settings", "assets/config/settings.json"},
		{"sounds", "assets/config/sounds.json"},
		{"units", "assets/config/units.json"},
		{"weapons", "assets/config/weapons.json"},
		{"spaceship", "assets/config/spaceships.json"}
	};

	void ensureHeadlessWindow()
	{
		if (!IsWindowReady())
		{
			SetConfigFlags(FLAG_WINDOW_HIDDEN);
			InitWindow(64, 64, "headless_turret_ship_test");
		}
	}

	struct CollisionRecorder
	{
		int collisionEventCount = 0;
		std::vector<event::CollisionEvent> recordedCollisions;

		void onCollision(const event::CollisionEvent &evt)
		{
			++collisionEventCount;
			recordedCollisions.push_back(evt);
		}
	};
}

TEST_CASE("Smoke test: Spaceship with collision model does not collide with or damage its own mounted turrets", "[smoke][collision][turret]")
{
	ensureHeadlessWindow();

	GameContext context;
	context.config.init(kSmokeConfigRoots);
	event::utils::hookAllListeners(context);

	CollisionRecorder recorder;
	context.dispatcher.sink<event::CollisionEvent>().connect<&CollisionRecorder::onCollision>(recorder);

	// Spawn player spaceship at origin with its collision model and 4 turrets
	spaceship::factory::SpawnParams shipParams;
	shipParams.position = Vector3Zeros;
	shipParams.radius = 3.0f;
	shipParams.faction = faction::FAC_BLUE;

	const spaceship::factory::SpawnedSpaceship ship =
		spaceship::factory::spawnConfiguredSpaceship(context, "player", shipParams);

	REQUIRE(context.registry.valid(ship.entity));
	REQUIRE(ship.turrets.size() == 4);

	// Verify the spaceship has a CollisionBodyModel loaded
	const auto *collisionModel = context.registry.try_get<collision::CollisionBodyModel>(ship.entity);
	REQUIRE(collisionModel != nullptr);

	// Record initial HP of all turrets
	std::vector<float> initialTurretHps;
	for (entt::entity turret : ship.turrets)
	{
		REQUIRE(context.registry.valid(turret));
		const auto *hp = context.registry.try_get<combat::HP>(turret);
		REQUIRE(hp != nullptr);
		initialTurretHps.push_back(hp->value);
	}

	// Run collision detection over multiple simulation frames
	systems::DetectEntityCollision collisionSystem;
	const float dt = 0.016f;

	for (int frame = 0; frame < 10; ++frame)
	{
		collisionSystem.update(context, dt);
		context.dispatcher.update();
	}

	// Verify zero collisions occurred between spaceship and turrets or between turrets
	CHECK(recorder.collisionEventCount == 0);
	CHECK(recorder.recordedCollisions.empty());

	// Verify all turret HPs remain untouched (player did not damage its own turrets)
	for (std::size_t i = 0; i < ship.turrets.size(); ++i)
	{
		const auto *hp = context.registry.try_get<combat::HP>(ship.turrets[i]);
		REQUIRE(hp != nullptr);
		CHECK(hp->value == Catch::Approx(initialTurretHps[i]));
	}

	// --- External Collision Test ---
	// Spawn an external foreign projectile sweeping across the ship hull
	const entt::entity externalBullet = context.registry.create();
	context.registry.emplace<physics::Position>(externalBullet, Vector3{0.0f, 0.0f, 1.5f});
	context.registry.emplace<physics::PrevPosition>(externalBullet, Vector3{0.0f, 0.0f, 3.5f});
	context.registry.emplace<physics::Velocity>(externalBullet, Vector3{0.0f, 0.0f, -120.0f});
	context.registry.emplace<collision::CollisionBody>(externalBullet, 0.5f);
	context.registry.emplace<combat::Damage>(externalBullet, 50.0f);
	context.registry.emplace<faction::Faction>(externalBullet, faction::FAC_RED);

	// Run one frame of collision detection
	collisionSystem.update(context, dt);
	context.dispatcher.update();

	// External entity must collide with the spaceship's collision model
	CHECK(recorder.collisionEventCount > 0);
	bool hitShip = false;
	for (const auto &evt : recorder.recordedCollisions)
	{
		if ((evt.a.id == ship.entity && evt.b.id == externalBullet) ||
			(evt.b.id == ship.entity && evt.a.id == externalBullet))
		{
			hitShip = true;
			break;
		}
	}
	CHECK(hitShip);

	// --- External Collision with Turret Test ---
	recorder.recordedCollisions.clear();
	recorder.collisionEventCount = 0;

	// Pick turret 0
	const entt::entity targetTurret = ship.turrets[0];
	const Vector3 turretPos = context.registry.get<physics::Position>(targetTurret).value;

	const entt::entity externalTurretHunter = context.registry.create();
	context.registry.emplace<physics::Position>(externalTurretHunter, turretPos);
	context.registry.emplace<physics::PrevPosition>(externalTurretHunter, turretPos + Vector3{0.0f, 1.5f, 0.0f});
	context.registry.emplace<physics::Velocity>(externalTurretHunter, Vector3{0.0f, -100.0f, 0.0f});
	context.registry.emplace<collision::CollisionBody>(externalTurretHunter, 0.5f);
	context.registry.emplace<combat::Damage>(externalTurretHunter, 25.0f);
	context.registry.emplace<faction::Faction>(externalTurretHunter, faction::FAC_RED);

	collisionSystem.update(context, dt);
	context.dispatcher.update();

	bool hitTurret = false;
	for (const auto &evt : recorder.recordedCollisions)
	{
		if ((evt.a.id == targetTurret && evt.b.id == externalTurretHunter) ||
			(evt.b.id == targetTurret && evt.a.id == externalTurretHunter))
		{
			hitTurret = true;
			break;
		}
	}
	CHECK(hitTurret);
}
