#include "entities.hpp"
#include "weapons.hpp"
#include "components/factions.hpp"

namespace {
	const int BASE_SCORE = 500;
	const float BASE_HP = 1000.0f;
	const float BASE_SHIELD = 500.0f;
	const float BASE_SHIELD_REGEN = BASE_SHIELD / 15;
}

entt::entity spawnBaseEnemy(GameContext &context, const Vector3& pos) {
	entt::entity enemy = context.registry.create();
	
	t_model_id shipModel = context.modelManager.loadModel("assets/Models/spacechip1/model/Intergalactic_Spaceship-(Wavefront).obj", 0.4f);
	context.registry.emplace<Position>(enemy, pos);
	context.registry.emplace<Velocity>(enemy);
	context.registry.emplace<Rotation>(enemy);
	context.registry.emplace<CollisionBody>(enemy, 1.0f);
	context.registry.emplace<RenderBody>(enemy, RenderBody{
		shipModel, GREEN, 1.0f
	});
	context.registry.emplace<HP>(enemy, BASE_HP);
	context.registry.emplace<EnergyShield>(enemy, BASE_SHIELD);
	context.registry.emplace<EnergyShieldRegen>(enemy, BASE_SHIELD_REGEN);
	context.registry.emplace<Damage>(enemy, 500.0f);
	context.registry.emplace<MaxSpeed>(enemy, 80.0f);
	context.registry.emplace<TurnSpeed>(enemy, 2.5f);
	context.registry.emplace<Score>(enemy);
	context.registry.emplace<KilledScore>(enemy, BASE_SCORE);
	context.registry.emplace<MoveTarget>(enemy);
	
	context.registry.emplace<faction::Faction>(enemy, faction::FAC_RED);
	context.registry.emplace<tag::Targetable>(enemy);
	context.registry.emplace<tag::Enemy>(enemy);
	context.registry.emplace<tag::Shaded>(enemy);
	context.registry.emplace<tag::RotationSyncModel>(enemy);
	context.registry.emplace<tag::effect::DropDebris>(enemy);
	context.registry.emplace<tag::effect::ExplodeOnDeath>(enemy);

	context.registry.emplace<tag::weapon::AIControlledAim>(enemy);
	context.registry.emplace<tag::weapon::AIControlledFire>(enemy);
	return enemy;
}

entt::entity spawnEnemy(GameContext &context, const Vector3& pos) {
	entt::entity enemy = spawnBaseEnemy(context, pos);

	weapon::emplaceWeaponBasic(context, enemy);
	return enemy;
}

entt::entity spawnEliteEnemy(GameContext &context, const Vector3& pos) {
	entt::entity enemy = spawnBaseEnemy(context, pos);

	float radius = 3.0f;
	context.registry.emplace_or_replace<CollisionBody>(enemy, radius);
	RenderBody &renderBody = context.registry.get<RenderBody>(enemy);
	renderBody.color = DARKGREEN;
	renderBody.scale = Vector3Ones * radius;
	context.registry.emplace_or_replace<HP>(enemy, 1200.0f);
	context.registry.emplace_or_replace<HPRegen>(enemy, 10.0f);
	context.registry.emplace_or_replace<EnergyShield>(enemy, 1000.0f);
	context.registry.emplace_or_replace<EnergyShieldRegen>(enemy, 25.0f);
	context.registry.emplace_or_replace<MaxSpeed>(enemy, 40.0f);
	context.registry.emplace_or_replace<KilledScore>(enemy, BASE_SCORE * 2);

	weapon::emplaceRandomWeapon(context, enemy);
	int subWeapons = GetRandomValue(0, 1000);
	weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, enemy, {+radius * 1.5f, 0, 0}), subWeapons);
	weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, enemy, {-radius * 1.5f, 0, 0}), subWeapons);

	context.registry.emplace_or_replace<tag::EliteUnit>(enemy);
	return enemy;
}

entt::entity spawnFastEliteEnemy(GameContext &context, const Vector3& pos) {
	entt::entity enemy = spawnBaseEnemy(context, pos);

	float radius = 2.0f;
	context.registry.emplace_or_replace<CollisionBody>(enemy, radius);
	RenderBody &renderBody = context.registry.get<RenderBody>(enemy);
	renderBody.color = LIME;
	renderBody.scale = Vector3Ones * radius;
	context.registry.emplace_or_replace<HP>(enemy, 720.0f);
	context.registry.emplace_or_replace<HPRegen>(enemy, 1.0f);
	context.registry.emplace_or_replace<MaxSpeed>(enemy, 160.0f);
	context.registry.emplace_or_replace<TurnSpeed>(enemy, 1.5f);
	context.registry.emplace_or_replace<KilledScore>(enemy, BASE_SCORE * 2);

	weapon::emplaceRandomWeapon(context, enemy);
	int subWeapons = GetRandomValue(0, 1000);
	weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, enemy, {+radius * 1.5f, 0, 0}), subWeapons);
	weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, enemy, {-radius * 1.5f, 0, 0}), subWeapons);

	context.registry.emplace_or_replace<tag::EliteUnit>(enemy);
	return enemy;
}

entt::entity spawnMothershipEnemy(GameContext &context, const Vector3& pos) {
	entt::entity enemy = spawnBaseEnemy(context, pos);

	float radius = 6.0f;
	context.registry.emplace_or_replace<CollisionBody>(enemy, radius);
	RenderBody &renderBody = context.registry.get<RenderBody>(enemy);
	renderBody.color = Color{191, 245, 66, 255};
	renderBody.scale = Vector3Ones * radius;
	context.registry.emplace_or_replace<HP>(enemy, 1600.0f);
	context.registry.emplace_or_replace<HPRegen>(enemy, 50.0f);
	context.registry.emplace_or_replace<EnergyShield>(enemy, 1000.0f);
	context.registry.emplace_or_replace<EnergyShieldRegen>(enemy, 100.0f);
	context.registry.emplace_or_replace<MaxSpeed>(enemy, 20.0f);
	context.registry.emplace_or_replace<TurnSpeed>(enemy, 0.25f);
	context.registry.emplace_or_replace<KilledScore>(enemy, BASE_SCORE * 5);

	weapon::emplaceRandomWeapon(context, enemy);
	weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, enemy, {+radius * 1.5f, +radius * 0.8f, -2}));
	weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, enemy, {+radius * 1.5f, -radius * 0.8f, -2}));
	weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, enemy, {-radius * 1.5f, +radius * 0.8f, -2}));
	weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, enemy, {-radius * 1.5f, -radius * 0.8f, -2}));
	// weapon::emplaceWeaponBasic(context, spawnLinkedTurret(context, renderBody.color, enemy, {+radius * 2.0f, +radius * 1.2f, -1}));
	// weapon::emplaceWeaponBasic(context, spawnLinkedTurret(context, renderBody.color, enemy, {+radius * 2.0f, -radius * 1.2f, -1}));
	// weapon::emplaceWeaponBasic(context, spawnLinkedTurret(context, renderBody.color, enemy, {-radius * 2.0f, +radius * 1.2f, -1}));
	// weapon::emplaceWeaponBasic(context, spawnLinkedTurret(context, renderBody.color, enemy, {-radius * 2.0f, -radius * 1.2f, -1}));
	context.registry.emplace_or_replace<tag::EliteUnit>(enemy);
	return enemy;
}