#include "catch2/catch_amalgamated.hpp"
#include "game_context.hpp"
#include "components/collision.hpp"
#include "components/identity.hpp"
#include "components/combat.hpp"
#include "components/physics.hpp"
#include "components/score.hpp"
#include "components/faction.hpp"
#include "systems.hpp"
#include "events.hpp"

namespace {
	struct CollisionRecorder {
		int count = 0;

		void onCollision(const event::CollisionEvent &) {
			++count;
		}
	};
}

TEST_CASE("Assembly: Ship and mounted turrets with same Assembly root do not collide", "[integration][assembly]") {
	GameContext context;
	systems::DetectEntityCollision detectCollision;

	CollisionRecorder recorder;
	context.dispatcher.sink<event::CollisionEvent>().connect<&CollisionRecorder::onCollision>(recorder);

	// Create spaceship with Assembly{ship}
	const entt::entity ship = context.registry.create();
	context.registry.emplace<physics::Position>(ship, Vector3{0.0f, 0.0f, 0.0f});
	context.registry.emplace<physics::Velocity>(ship, Vector3{0.0f, 0.0f, 0.0f});
	context.registry.emplace<collision::CollisionBody>(ship, 10.0f);
	context.registry.emplace<collision::Assembly>(ship, ship);
	context.registry.emplace<faction::Faction>(ship, faction::FAC_BLUE);

	// Create mounted turret with Assembly{ship} physically co-located on hull
	const entt::entity turret = context.registry.create();
	context.registry.emplace<physics::Position>(turret, Vector3{5.0f, 0.0f, 0.0f});
	context.registry.emplace<physics::Velocity>(turret, Vector3{0.0f, 0.0f, 0.0f});
	context.registry.emplace<collision::CollisionBody>(turret, 3.0f);
	context.registry.emplace<collision::Assembly>(turret, ship);

	// Create sibling turret with Assembly{ship} overlapping the first turret
	const entt::entity turret2 = context.registry.create();
	context.registry.emplace<physics::Position>(turret2, Vector3{6.0f, 0.0f, 0.0f});
	context.registry.emplace<physics::Velocity>(turret2, Vector3{0.0f, 0.0f, 0.0f});
	context.registry.emplace<collision::CollisionBody>(turret2, 3.0f);
	context.registry.emplace<collision::Assembly>(turret2, ship);

	detectCollision.update(context, 0.016f);
	context.dispatcher.update();
	CHECK(recorder.count == 0);

	// Now introduce an external hostile entity without Assembly
	const entt::entity hostileEntity = context.registry.create();
	context.registry.emplace<physics::Position>(hostileEntity, Vector3{2.0f, 0.0f, 0.0f});
	context.registry.emplace<physics::PrevPosition>(hostileEntity, Vector3{10.0f, 0.0f, 0.0f});
	context.registry.emplace<collision::CollisionBody>(hostileEntity, 1.0f);

	detectCollision.update(context, 0.016f);
	context.dispatcher.update();
	CHECK(recorder.count > 0);
}

TEST_CASE("Owner: Kill attribution and score transfer using flat identity::Owner", "[integration][owner]") {
	GameContext context;
	event::Listener listener;

	// Create player spaceship with Owner{player} and Score
	const entt::entity player = context.registry.create();
	context.registry.emplace<identity::Owner>(player, player);
	context.registry.emplace<score::Score>(player, 0);
	context.currentPlayer = player;

	// Create mounted turret with Owner{player}
	const entt::entity turret = context.registry.create();
	context.registry.emplace<identity::Owner>(turret, player);

	// Create bullet fired by turret, inheriting Owner{player}
	const entt::entity bullet = context.registry.create();
	context.registry.emplace<identity::Owner>(bullet, player);

	// Create enemy with KilledScore
	const entt::entity enemy = context.registry.create();
	context.registry.emplace<score::KilledScore>(enemy, 500);

	// Bullet kills enemy
	event::KillEvent killEvt{
		&context,
		event::CollisionParty{bullet, Vector3Zeros, Vector3Zeros},
		event::CollisionParty{enemy, Vector3Zeros, Vector3Zeros},
		0.016f
	};
	listener.handleKillEvent(killEvt);

	// Verify score is awarded to player
	const auto &playerScore = context.registry.get<score::Score>(player);
	CHECK(playerScore.value == 500);

	// Verify loop immunity: scalar read does not loop even if entities reference each other
	const entt::entity entityA = context.registry.create();
	const entt::entity entityB = context.registry.create();
	context.registry.emplace<identity::Owner>(entityA, entityB);
	context.registry.emplace<identity::Owner>(entityB, entityA);

	const auto *ownerA = context.registry.try_get<identity::Owner>(entityA);
	const entt::entity rootA = ownerA ? ownerA->root : entityA;
	CHECK(rootA == entityB); // Direct O(1) resolution without graph traversal
}
