#include "catch2/catch_amalgamated.hpp"
#include "classes/hud_manager.hpp"
#include "game_context.hpp"
#include "components/unit.hpp"
#include "components/score.hpp"
#include "components/combat.hpp"
#include "components/physics.hpp"
#include "events.hpp"

TEST_CASE("HudManager: Request queue drains on update", "[integration][hud]") {
	GameContext context;
	HudManager &hud = context.hudManager;
	const entt::entity attacker = context.registry.create();
	const entt::entity target = context.registry.create();

	hud.setObservedEntity(attacker);
	hud.reportDamage(attacker, target, 45.0f, Vector3{0.0f, 0.0f, 0.0f}, HitType::NORMAL);

	CHECK(hud.getActiveDamageNumberCount() == 0);
	hud.update(0.016f, context);
	CHECK(hud.getActiveDamageNumberCount() == 1);
	CHECK(hud.getActiveDamageNumbers()[0].totalDamage == Catch::Approx(45.0f));
	CHECK(hud.getActiveDamageNumbers()[0].hitType == HitType::NORMAL);
}

TEST_CASE("HudManager: Damage aggregation sums rapid hits on same target", "[integration][hud]") {
	GameContext context;
	HudManager &hud = context.hudManager;
	const entt::entity attacker = context.registry.create();
	const entt::entity target = context.registry.create();

	hud.setObservedEntity(attacker);

	// 10 rapid laser shots within 0.1s
	for (int i = 0; i < 10; ++i) {
		hud.reportDamage(attacker, target, 12.0f, Vector3{0.0f, 0.0f, 0.0f}, HitType::NORMAL);
	}
	hud.update(0.05f, context);

	REQUIRE(hud.getActiveDamageNumberCount() == 1);
	CHECK(hud.getActiveDamageNumbers()[0].totalDamage == Catch::Approx(120.0f));
	CHECK(hud.getActiveDamageNumbers()[0].scale > 1.0f);
	CHECK(hud.getActiveDamageNumbers()[0].scale <= 1.25f);
}

TEST_CASE("HudManager: New damage after aggregation window replaces old damage number immediately", "[integration][hud]") {
	GameContext context;
	HudManager &hud = context.hudManager;
	const entt::entity attacker = context.registry.create();
	const entt::entity target = context.registry.create();

	hud.setObservedEntity(attacker);

	// Initial hit at pos1
	hud.reportDamage(attacker, target, 50.0f, Vector3{0.0f, 0.0f, 0.0f}, HitType::NORMAL);
	hud.update(0.016f, context);

	REQUIRE(hud.getActiveDamageNumberCount() == 1);
	CHECK(hud.getActiveDamageNumbers()[0].worldPos.x == Catch::Approx(0.0f));
	CHECK(hud.getActiveDamageNumbers()[0].totalDamage == Catch::Approx(50.0f));

	// Advance past aggregation window (0.45s)
	hud.update(0.5f, context);
	REQUIRE(hud.getActiveDamageNumberCount() == 1);

	// New hit at pos2 (enemy moved to x=25.0f)
	hud.reportDamage(attacker, target, 75.0f, Vector3{25.0f, 0.0f, 0.0f}, HitType::NORMAL);
	hud.update(0.016f, context);

	// The old damage number at x=0 must disappear immediately, replaced by the new one at x=25.0f
	REQUIRE(hud.getActiveDamageNumberCount() == 1);
	CHECK(hud.getActiveDamageNumbers()[0].worldPos.x == Catch::Approx(25.0f));
	CHECK(hud.getActiveDamageNumbers()[0].totalDamage == Catch::Approx(75.0f));
}

TEST_CASE("HudManager: HitType escalation to KILL", "[integration][hud]") {
	GameContext context;
	HudManager &hud = context.hudManager;
	const entt::entity attacker = context.registry.create();
	const entt::entity target = context.registry.create();

	hud.setObservedEntity(attacker);
	hud.reportDamage(attacker, target, 50.0f, Vector3{0.0f, 0.0f, 0.0f}, HitType::NORMAL);
	hud.update(0.016f, context);

	CHECK(hud.getActiveDamageNumbers()[0].hitType == HitType::NORMAL);

	// Lethal final hit
	hud.reportDamage(attacker, target, 50.0f, Vector3{0.0f, 0.0f, 0.0f}, HitType::KILL);
	hud.update(0.016f, context);

	REQUIRE(hud.getActiveDamageNumberCount() == 1);
	CHECK(hud.getActiveDamageNumbers()[0].totalDamage == Catch::Approx(100.0f));
	CHECK(hud.getActiveDamageNumbers()[0].hitType == HitType::KILL);
}

TEST_CASE("HudManager: Attacker filtering drops third-party damage", "[integration][hud]") {
	GameContext context;
	HudManager &hud = context.hudManager;
	const entt::entity player = context.registry.create();
	const entt::entity enemyA = context.registry.create();
	const entt::entity enemyB = context.registry.create();

	hud.setObservedEntity(player);

	// Third party combat: Enemy A damages Enemy B
	hud.reportDamage(enemyA, enemyB, 80.0f, Vector3{10.0f, 0.0f, 0.0f}, HitType::NORMAL);
	hud.update(0.016f, context);

	// Must be dropped
	CHECK(hud.getActiveDamageNumberCount() == 0);

	// Player damages Enemy B
	hud.reportDamage(player, enemyB, 60.0f, Vector3{10.0f, 0.0f, 0.0f}, HitType::CRITICAL);
	hud.update(0.016f, context);

	REQUIRE(hud.getActiveDamageNumberCount() == 1);
	CHECK(hud.getActiveDamageNumbers()[0].hitType == HitType::CRITICAL);
}

TEST_CASE("HudManager: Damage numbers expire after duration", "[integration][hud]") {
	GameContext context;
	HudManager &hud = context.hudManager;
	const entt::entity attacker = context.registry.create();
	const entt::entity target = context.registry.create();

	hud.setObservedEntity(attacker);
	hud.reportDamage(attacker, target, 25.0f, Vector3{0.0f, 0.0f, 0.0f}, HitType::NORMAL);
	hud.update(0.016f, context);
	CHECK(hud.getActiveDamageNumberCount() == 1);

	// Advance past 1.0s max duration
	hud.update(1.2f, context);
	CHECK(hud.getActiveDamageNumberCount() == 0);
}

TEST_CASE("HudManager: Left log respects max entries and top notif priority", "[integration][hud]") {
	GameContext context;
	HudManager &hud = context.hudManager;

	// Add 7 left log messages
	for (int i = 1; i <= 7; ++i) {
		hud.addToastLeftLog("Kill message " + std::to_string(i), GREEN);
	}
	hud.update(0.016f, context);

	// Max 5 left log entries
	size_t leftLogCount = 0;
	for (const auto &t : hud.getActiveToasts()) {
		if (t.slot == ToastSlot::LEFT_LOG)
			leftLogCount++;
	}
	CHECK(leftLogCount == 5);

	// Top notification priority preemption
	hud.addToastTopNotif("NORMAL ALERT", ToastPriority::NORMAL, YELLOW);
	hud.update(0.016f, context);
	hud.addToastTopNotif("LOW HP WARNING", ToastPriority::CRITICAL, RED);
	hud.update(0.016f, context);

	// The critical alert replaces the normal alert
	bool foundCritical = false;
	for (const auto &t : hud.getActiveToasts()) {
		if (t.slot == ToastSlot::TOP_NOTIF) {
			if (t.priority == ToastPriority::CRITICAL && t.text == "LOW HP WARNING")
				foundCritical = true;
		}
	}
	CHECK(foundCritical);
}

TEST_CASE("Events: Bullet destruction does not trigger killed by toast", "[integration][hud]") {
	GameContext context;
	const entt::entity player = context.registry.create();
	context.registry.emplace<tag::Spaceship>(player);
	context.registry.emplace<Name>(player, "Player");
	context.currentPlayer = player;
	context.hudManager.setObservedEntity(player);

	const entt::entity enemy = context.registry.create();
	context.registry.emplace<tag::Spaceship>(enemy);
	context.registry.emplace<Name>(enemy, "Mothership");

	// Player fires bullet
	const entt::entity bullet = context.registry.create();
	context.registry.emplace<tag::Bullet>(bullet);
	context.registry.emplace<ScoreParent>(bullet, player);

	// Bullet impacts enemy and bullet dies
	event::KillEvent bulletKilledEvt{
		&context,
		event::CollisionParty{enemy, Vector3Zeros, Vector3Zeros},
		event::CollisionParty{bullet, Vector3Zeros, Vector3Zeros},
		0.016f
	};
	event::Listener listener;
	listener.handleKillEvent(bulletKilledEvt);
	context.hudManager.update(0.016f, context);

	// No toasts should be generated when bullet dies!
	CHECK(context.hudManager.getActiveToastCount() == 0);

	// Enemy is killed by player's bullet
	event::KillEvent enemyKilledEvt{
		&context,
		event::CollisionParty{bullet, Vector3Zeros, Vector3Zeros},
		event::CollisionParty{enemy, Vector3Zeros, Vector3Zeros},
		0.016f
	};
	listener.handleKillEvent(enemyKilledEvt);
	context.hudManager.update(0.016f, context);

	// Should generate "Killed Mothership" toast!
	REQUIRE(context.hudManager.getActiveToastCount() == 1);
	CHECK(context.hudManager.getActiveToasts()[0].text == "Killed Mothership");
}

TEST_CASE("Combat: Two bullets hitting same target in same tick triggers exactly 1 kill", "[integration][hud]") {
	GameContext context;
	const entt::entity player = context.registry.create();
	context.registry.emplace<tag::Spaceship>(player);
	context.registry.emplace<Name>(player, "fighter");
	context.currentPlayer = player;
	context.hudManager.setObservedEntity(player);

	const entt::entity target = context.registry.create();
	context.registry.emplace<tag::Spaceship>(target);
	context.registry.emplace<tag::Targetable>(target);
	context.registry.emplace<Name>(target, "basic");
	context.registry.emplace<HP>(target, HP{50.0f});

	const entt::entity bulletA = context.registry.create();
	context.registry.emplace<tag::Bullet>(bulletA);
	context.registry.emplace<Damage>(bulletA, Damage{40.0f});
	context.registry.emplace<ScoreParent>(bulletA, player);

	const entt::entity bulletB = context.registry.create();
	context.registry.emplace<tag::Bullet>(bulletB);
	context.registry.emplace<Damage>(bulletB, Damage{40.0f});
	context.registry.emplace<ScoreParent>(bulletB, player);

	event::Listener listener;
	event::utils::hookAllListeners(context);

	// First bullet hits: 50 -> 10 HP
	event::CollisionEvent hitA{&context, event::CollisionParty{bulletA, Vector3Zeros, Vector3Zeros}, event::CollisionParty{target, Vector3Zeros, Vector3Zeros}, 0.016f, 0.0f};
	listener.handleCollisionEvent(hitA);

	// Second bullet hits in the same tick: 10 -> -30 HP (killing blow)
	event::CollisionEvent hitB{&context, event::CollisionParty{bulletB, Vector3Zeros, Vector3Zeros}, event::CollisionParty{target, Vector3Zeros, Vector3Zeros}, 0.016f, 0.0f};
	listener.handleCollisionEvent(hitB);

	// Third overkill bullet hits dead target: should be ignored
	const entt::entity bulletC = context.registry.create();
	context.registry.emplace<tag::Bullet>(bulletC);
	context.registry.emplace<Damage>(bulletC, Damage{40.0f});
	context.registry.emplace<ScoreParent>(bulletC, player);

	event::CollisionEvent hitC{&context, event::CollisionParty{bulletC, Vector3Zeros, Vector3Zeros}, event::CollisionParty{target, Vector3Zeros, Vector3Zeros}, 0.016f, 0.0f};
	listener.handleCollisionEvent(hitC);

	context.dispatcher.update();
	context.hudManager.update(0.016f, context);

	REQUIRE(context.hudManager.getActiveToastCount() == 1);
	CHECK(context.hudManager.getActiveToasts()[0].text == "Killed basic");
	REQUIRE(context.hudManager.getActiveDamageNumberCount() == 1);
	CHECK(context.hudManager.getActiveDamageNumbers()[0].totalDamage == Catch::Approx(80.0f));
	CHECK(context.hudManager.getActiveDamageNumbers()[0].hitType == HitType::KILL);
}

TEST_CASE("HudManager: Simultaneous multi-target damage spawns separate numbers per target", "[integration][hud]") {
	GameContext context;
	const entt::entity player = context.registry.create();
	context.currentPlayer = player;
	context.hudManager.setObservedEntity(player);

	const entt::entity targetA = context.registry.create();
	const entt::entity targetB = context.registry.create();

	context.hudManager.reportDamage(player, targetA, 150.0f, Vector3{10.0f, 0.0f, 0.0f}, HitType::NORMAL);
	context.hudManager.reportDamage(player, targetB, 200.0f, Vector3{-10.0f, 0.0f, 0.0f}, HitType::CRITICAL);
	context.hudManager.update(0.016f, context);

	REQUIRE(context.hudManager.getActiveDamageNumberCount() == 2);
	CHECK(context.hudManager.getActiveDamageNumbers()[0].target == targetA);
	CHECK(context.hudManager.getActiveDamageNumbers()[0].totalDamage == Catch::Approx(150.0f));
	CHECK(context.hudManager.getActiveDamageNumbers()[0].worldPos.x == Catch::Approx(10.0f));
	CHECK(context.hudManager.getActiveDamageNumbers()[0].hitType == HitType::NORMAL);

	CHECK(context.hudManager.getActiveDamageNumbers()[1].target == targetB);
	CHECK(context.hudManager.getActiveDamageNumbers()[1].totalDamage == Catch::Approx(200.0f));
	CHECK(context.hudManager.getActiveDamageNumbers()[1].worldPos.x == Catch::Approx(-10.0f));
	CHECK(context.hudManager.getActiveDamageNumbers()[1].hitType == HitType::CRITICAL);
}

TEST_CASE("HudManager: Simultaneous multi-kill stacks multiple toasts in left log", "[integration][hud]") {
	GameContext context;
	const entt::entity player = context.registry.create();
	context.registry.emplace<tag::Spaceship>(player);
	context.currentPlayer = player;
	context.hudManager.setObservedEntity(player);

	const entt::entity targetA = context.registry.create();
	context.registry.emplace<tag::Spaceship>(targetA);
	context.registry.emplace<Name>(targetA, "basic");

	const entt::entity targetB = context.registry.create();
	context.registry.emplace<tag::Spaceship>(targetB);
	context.registry.emplace<Name>(targetB, "elite");

	event::Listener listener;
	event::utils::hookAllListeners(context);

	listener.handleKillEvent(event::KillEvent{&context, event::CollisionParty{player, Vector3Zeros, Vector3Zeros}, event::CollisionParty{targetA, Vector3Zeros, Vector3Zeros}, 0.016f});
	listener.handleKillEvent(event::KillEvent{&context, event::CollisionParty{player, Vector3Zeros, Vector3Zeros}, event::CollisionParty{targetB, Vector3Zeros, Vector3Zeros}, 0.016f});
	context.hudManager.update(0.016f, context);

	REQUIRE(context.hudManager.getActiveToastCount() == 2);
	CHECK(context.hudManager.getActiveToasts()[0].text == "Killed basic");
	CHECK(context.hudManager.getActiveToasts()[1].text == "Killed elite");
	for (const auto &toast : context.hudManager.getActiveToasts()) {
		CHECK(toast.slot == ToastSlot::LEFT_LOG);
	}
}

TEST_CASE("Settings: Damage numbers, toasts, and kill logs configuration toggles", "[integration][hud][settings]") {
	GameContext context;
	context.config.init({{"settings", "assets/config/settings.json"}});
	CHECK(context.config.settings.showDamageNumbers == true);
	CHECK(context.config.settings.showToasts == true);
	CHECK(context.config.settings.showKillLogs == true);

	context.config.setBool("settings.showDamageNumbers", false);
	CHECK(context.config.settings.showDamageNumbers == false);
	context.config.setBool("settings.showToasts", false);
	CHECK(context.config.settings.showToasts == false);
	context.config.setBool("settings.showKillLogs", false);
	CHECK(context.config.settings.showKillLogs == false);
}

TEST_CASE("Combat: Damage numbers generated only for Targetable victims", "[integration][hud]") {
	GameContext context;
	const entt::entity player = context.registry.create();
	context.registry.emplace<tag::Spaceship>(player);
	context.registry.emplace<HP>(player, 1000.0f);
	context.registry.emplace<Damage>(player, 500.0f);
	context.currentPlayer = player;
	context.hudManager.setObservedEntity(player);

	const entt::entity untargetableObj = context.registry.create();
	context.registry.emplace<tag::Bullet>(untargetableObj);
	context.registry.emplace<HP>(untargetableObj, 1.0f);

	const entt::entity enemyShip = context.registry.create();
	context.registry.emplace<tag::Spaceship>(enemyShip);
	context.registry.emplace<tag::Targetable>(enemyShip);
	context.registry.emplace<HP>(enemyShip, 100.0f);

	event::Listener listener;

	// Collision with untargetable object
	listener.handleCollisionEvent(event::CollisionEvent{&context, event::CollisionParty{player, Vector3Zeros, Vector3Zeros}, event::CollisionParty{untargetableObj, Vector3Zeros, Vector3Zeros}, 0.016f, 1.0f});
	context.hudManager.update(0.016f, context);
	CHECK(context.hudManager.getActiveDamageNumberCount() == 0);

	// Collision with targetable enemy spaceship
	listener.handleCollisionEvent(event::CollisionEvent{&context, event::CollisionParty{player, Vector3Zeros, Vector3Zeros}, event::CollisionParty{enemyShip, Vector3Zeros, Vector3Zeros}, 0.016f, 1.0f});
	context.hudManager.update(0.016f, context);
	CHECK(context.hudManager.getActiveDamageNumberCount() == 1);
}

TEST_CASE("Config: HUD damage numbers configuration loads from hud.json", "[integration][hud][config]") {
	GameContext context;
	context.config.init({{"hud", "assets/config/hud.json"}});

	CHECK(context.config.hud.damageNumbers.fontSize == Catch::Approx(20.0f));
	CHECK(context.config.hud.damageNumbers.opacity == Catch::Approx(0.85f));
	CHECK(context.config.hud.damageNumbers.resetCooldown == Catch::Approx(1.5f));

	// Test dynamic reset cooldown effect on aggregation
	context.config.hud.damageNumbers.resetCooldown = 0.2f;
	const entt::entity attacker = context.registry.create();
	const entt::entity target = context.registry.create();
	context.hudManager.setObservedEntity(attacker);

	context.hudManager.reportDamage(attacker, target, 50.0f, Vector3{0.0f, 0.0f, 0.0f}, HitType::NORMAL);
	context.hudManager.update(0.016f, context);

	// At 0.25s, it is past 0.2f cooldown, so next hit triggers new text (replaces old)
	context.hudManager.update(0.25f, context);
	context.hudManager.reportDamage(attacker, target, 80.0f, Vector3{10.0f, 0.0f, 0.0f}, HitType::NORMAL);
	context.hudManager.update(0.016f, context);

	REQUIRE(context.hudManager.getActiveDamageNumberCount() == 1);
	CHECK(context.hudManager.getActiveDamageNumbers()[0].totalDamage == Catch::Approx(80.0f));
	CHECK(context.hudManager.getActiveDamageNumbers()[0].worldPos.x == Catch::Approx(10.0f));
}

TEST_CASE("DamageContributors: LRU eviction and last damage dealer", "[integration][hud][combat]") {
	DamageContributors dc;
	const entt::entity e1 = static_cast<entt::entity>(1);
	const entt::entity e2 = static_cast<entt::entity>(2);
	const entt::entity e3 = static_cast<entt::entity>(3);
	const entt::entity e4 = static_cast<entt::entity>(4);

	dc.recordDamage(e1, 10.0f, 1.0f);
	dc.recordDamage(e2, 20.0f, 2.0f);
	dc.recordDamage(e3, 30.0f, 3.0f);
	CHECK(dc.count == 3);
	CHECK(dc.getTotalDamage() == Catch::Approx(60.0f));

	// Evict LRU (e1 at t=1.0)
	dc.recordDamage(e4, 40.0f, 4.0f);
	CHECK(dc.count == 3);
	CHECK(dc.getLastDamageDealer(5.0f) == e4);

	// e1 should no longer be present
	bool hasE1 = false;
	for (size_t i = 0; i < dc.count; ++i) {
		if (dc.entries[i].attacker == e1)
			hasE1 = true;
	}
	CHECK_FALSE(hasE1);

	// Exclude e4
	CHECK(dc.getLastDamageDealer(5.0f, 10.0f, e4) == e3);
}

TEST_CASE("Events: Kill assist toast awarded when player contributes >=30% within 10s", "[integration][hud]") {
	GameContext context;
	const entt::entity player = context.registry.create();
	context.registry.emplace<tag::Spaceship>(player);
	context.registry.emplace<Name>(player, "Player");
	context.currentPlayer = player;
	context.hudManager.setObservedEntity(player);

	const entt::entity ally = context.registry.create();
	context.registry.emplace<tag::Spaceship>(ally);
	context.registry.emplace<Name>(ally, "Ally");

	const entt::entity enemy = context.registry.create();
	context.registry.emplace<tag::Spaceship>(enemy);
	context.registry.emplace<Name>(enemy, "Scout");
	context.registry.emplace<HP>(enemy, 100.0f);

	auto &dc = context.registry.emplace<DamageContributors>(enemy);
	dc.recordDamage(player, 40.0f, 1.0f); // 40% of 100 HP
	context.gameTime = 3.0f;              // 2s later (<10s)

	event::Listener listener;
	listener.handleKillEvent(event::KillEvent{&context, event::CollisionParty{ally, Vector3Zeros, Vector3Zeros}, event::CollisionParty{enemy, Vector3Zeros, Vector3Zeros}, 0.016f});

	context.hudManager.update(0.016f, context);
	const auto &toasts = context.hudManager.getActiveToasts();
	bool foundAssist = false;
	for (const auto &t : toasts) {
		if (t.slot == ToastSlot::LEFT_LOG && t.text == "Kill assist Scout" && t.color.b == SKYBLUE.b)
			foundAssist = true;
	}
	CHECK(foundAssist);
}

TEST_CASE("Events: Kill assist toast not awarded when damage < 30%", "[integration][hud]") {
	GameContext context;
	const entt::entity player = context.registry.create();
	context.registry.emplace<tag::Spaceship>(player);
	context.registry.emplace<Name>(player, "Player");
	context.currentPlayer = player;
	context.hudManager.setObservedEntity(player);

	const entt::entity ally = context.registry.create();
	context.registry.emplace<tag::Spaceship>(ally);

	const entt::entity enemy = context.registry.create();
	context.registry.emplace<tag::Spaceship>(enemy);
	context.registry.emplace<Name>(enemy, "Drone");
	context.registry.emplace<HP>(enemy, 100.0f);

	auto &dc = context.registry.emplace<DamageContributors>(enemy);
	dc.recordDamage(player, 15.0f, 1.0f);
	dc.recordDamage(ally, 85.0f, 2.0f);
	context.gameTime = 2.0f;

	event::Listener listener;
	listener.handleKillEvent(event::KillEvent{&context, event::CollisionParty{ally, Vector3Zeros, Vector3Zeros}, event::CollisionParty{enemy, Vector3Zeros, Vector3Zeros}, 0.016f});
	context.hudManager.update(0.016f, context);

	for (const auto &t : context.hudManager.getActiveToasts()) {
		CHECK(t.text != "Kill assist Drone");
	}
}

TEST_CASE("Events: Kill assist toast not awarded when expired > 10s", "[integration][hud]") {
	GameContext context;
	const entt::entity player = context.registry.create();
	context.registry.emplace<tag::Spaceship>(player);
	context.registry.emplace<Name>(player, "Player");
	context.currentPlayer = player;
	context.hudManager.setObservedEntity(player);

	const entt::entity ally = context.registry.create();
	context.registry.emplace<tag::Spaceship>(ally);

	const entt::entity enemy = context.registry.create();
	context.registry.emplace<tag::Spaceship>(enemy);
	context.registry.emplace<Name>(enemy, "Drone");
	context.registry.emplace<HP>(enemy, 100.0f);

	auto &dc = context.registry.emplace<DamageContributors>(enemy);
	dc.recordDamage(player, 50.0f, 1.0f);
	dc.recordDamage(ally, 50.0f, 15.0f);
	context.gameTime = 15.0f; // 14s elapsed since player hit

	event::Listener listener;
	listener.handleKillEvent(event::KillEvent{&context, event::CollisionParty{ally, Vector3Zeros, Vector3Zeros}, event::CollisionParty{enemy, Vector3Zeros, Vector3Zeros}, 0.016f});
	context.hudManager.update(0.016f, context);

	for (const auto &t : context.hudManager.getActiveToasts()) {
		CHECK(t.text != "Kill assist Drone");
	}
}

TEST_CASE("Events: Asteroid death attributes kill to player if damaged within 10s", "[integration][hud]") {
	GameContext context;
	const entt::entity player = context.registry.create();
	context.registry.emplace<tag::Spaceship>(player);
	context.registry.emplace<Name>(player, "Player");
	context.currentPlayer = player;
	context.hudManager.setObservedEntity(player);

	const entt::entity asteroid = context.registry.create();
	context.registry.emplace<tag::Asteroid>(asteroid);

	const entt::entity enemy = context.registry.create();
	context.registry.emplace<tag::Spaceship>(enemy);
	context.registry.emplace<Name>(enemy, "Raider");
	context.registry.emplace<HP>(enemy, 100.0f);

	auto &dc = context.registry.emplace<DamageContributors>(enemy);
	dc.recordDamage(player, 25.0f, 5.0f);
	context.gameTime = 7.0f;

	event::Listener listener;
	listener.handleKillEvent(event::KillEvent{&context, event::CollisionParty{asteroid, Vector3Zeros, Vector3Zeros}, event::CollisionParty{enemy, Vector3Zeros, Vector3Zeros}, 0.016f});

	context.hudManager.update(0.016f, context);
	bool foundKill = false;
	for (const auto &t : context.hudManager.getActiveToasts()) {
		if (t.slot == ToastSlot::LEFT_LOG && t.text == "Killed Raider")
			foundKill = true;
	}
	CHECK(foundKill);
}

TEST_CASE("Events: Unknown player death shows 'You have been killed'", "[integration][hud]") {
	GameContext context;
	const entt::entity player = context.registry.create();
	context.registry.emplace<tag::Spaceship>(player);
	context.registry.emplace<Name>(player, "Player");
	context.currentPlayer = player;
	context.hudManager.setObservedEntity(player);

	// Player dies to null entity
	event::Listener listener;
	listener.handleKillEvent(event::KillEvent{&context, event::CollisionParty{entt::null, Vector3Zeros, Vector3Zeros}, event::CollisionParty{player, Vector3Zeros, Vector3Zeros}, 0.016f});

	context.hudManager.update(0.016f, context);
	bool foundDeathToast = false;
	for (const auto &t : context.hudManager.getActiveToasts()) {
		if (t.slot == ToastSlot::LEFT_LOG && t.text == "You have been killed" && t.color.r == RED.r)
			foundDeathToast = true;
	}
	CHECK(foundDeathToast);
}

TEST_CASE("HudManager: Damage number tracks moving target entity position every frame", "[integration][hud]") {
	GameContext context;
	HudManager &hud = context.hudManager;
	const entt::entity attacker = context.registry.create();
	const entt::entity target = context.registry.create();
	context.registry.emplace<Position>(target, Position{Vector3{10.0f, 20.0f, 30.0f}});
	hud.setObservedEntity(attacker);

	// Hit at target's position
	hud.reportDamage(attacker, target, 50.0f, Vector3{10.0f, 20.0f, 30.0f}, HitType::NORMAL);
	hud.update(0.016f, context);

	REQUIRE(hud.getActiveDamageNumberCount() == 1);
	CHECK(hud.getActiveDamageNumbers()[0].worldPos.x == Catch::Approx(10.0f));

	// Target moves 40 units along X in the next frame
	context.registry.get<Position>(target).value = Vector3{50.0f, 20.0f, 30.0f};
	hud.update(0.016f, context);

	// Damage number must follow target to X = 50.0f
	REQUIRE(hud.getActiveDamageNumberCount() == 1);
	CHECK(hud.getActiveDamageNumbers()[0].worldPos.x == Catch::Approx(50.0f));
	CHECK(hud.getActiveDamageNumbers()[0].worldPos.z == Catch::Approx(30.0f));
	// Y floats upwards relative to target
	CHECK(hud.getActiveDamageNumbers()[0].worldPos.y > 20.0f);
}

TEST_CASE("HudManager: Destroyed target damage number continues floating from last position", "[integration][hud]") {
	GameContext context;
	HudManager &hud = context.hudManager;
	const entt::entity attacker = context.registry.create();
	const entt::entity target = context.registry.create();
	context.registry.emplace<Position>(target, Position{Vector3{100.0f, 0.0f, 0.0f}});
	hud.setObservedEntity(attacker);

	hud.reportDamage(attacker, target, 50.0f, Vector3{100.0f, 0.0f, 0.0f}, HitType::NORMAL);
	hud.update(0.016f, context);
	CHECK(hud.getActiveDamageNumbers()[0].worldPos.x == Catch::Approx(100.0f));

	// Destroy the target entity
	context.registry.destroy(target);

	// Update hud
	hud.update(0.016f, context);

	// Damage number still exists, stays at X = 100.0f, floats in Y
	REQUIRE(hud.getActiveDamageNumberCount() == 1);
	CHECK(hud.getActiveDamageNumbers()[0].worldPos.x == Catch::Approx(100.0f));
	CHECK(hud.getActiveDamageNumbers()[0].worldPos.y > 0.0f);
}


