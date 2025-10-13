#include "entities.hpp"
#include "weapons.hpp"

namespace {
	const int BASE_SCORE = 500;
	const float BASE_HP = 1000.0f;
	const float BASE_SHIELD = 500.0f;
	const float BASE_SHIELD_REGEN = BASE_SHIELD / 15;
}

entt::entity spawnBaseUnit(GameContext &context, const Vector3& pos) {
	entt::entity entity = context.registry.create();
	
	t_model_id shipModel = context.modelManager.loadModel("assets/Models/spacechip1/model/Intergalactic_Spaceship-(Wavefront).obj", 0.4f);
	context.registry.emplace<Position>(entity, pos);
	context.registry.emplace<Velocity>(entity);
	context.registry.emplace<Rotation>(entity);
	context.registry.emplace<CollisionBody>(entity, 1.0f);
	context.registry.emplace<RenderBody>(entity, RenderBody{
		shipModel, WHITE, 1.0f
	});
	context.registry.emplace<HP>(entity, BASE_HP);
	context.registry.emplace<EnergyShield>(entity, BASE_SHIELD);
	context.registry.emplace<EnergyShieldRegen>(entity, BASE_SHIELD_REGEN);
	context.registry.emplace<Damage>(entity, 500.0f);
	context.registry.emplace<MaxSpeed>(entity, 80.0f);
	context.registry.emplace<TurnSpeed>(entity, 2.5f);
	context.registry.emplace<Score>(entity);
	context.registry.emplace<KilledScore>(entity, BASE_SCORE);
	context.registry.emplace<MoveTarget>(entity);
	
	context.registry.emplace<tag::Targetable>(entity);
	context.registry.emplace<tag::Shaded>(entity);
	context.registry.emplace<tag::RotationSyncModel>(entity);
	context.registry.emplace<tag::effect::DropDebris>(entity);
	context.registry.emplace<tag::effect::ExplodeOnDeath>(entity);

	context.registry.emplace<tag::weapon::AIControlledAim>(entity);
	context.registry.emplace<tag::weapon::AIControlledFire>(entity);
	return entity;
}

entt::entity spawnUnit(GameContext &context, const Vector3& pos) {
	entt::entity entity = spawnBaseUnit(context, pos);

	weapon::emplaceWeaponBasic(context, entity);
	return entity;
}

entt::entity spawnEliteUnit(GameContext &context, const Vector3& pos) {
	entt::entity entity = spawnBaseUnit(context, pos);

	float radius = 3.0f;
	context.registry.emplace_or_replace<CollisionBody>(entity, radius);
	RenderBody &renderBody = context.registry.get<RenderBody>(entity);
	renderBody.scale = Vector3Ones * radius;
	context.registry.emplace_or_replace<HP>(entity, 1200.0f);
	context.registry.emplace_or_replace<HPRegen>(entity, 10.0f);
	context.registry.emplace_or_replace<EnergyShield>(entity, 1000.0f);
	context.registry.emplace_or_replace<EnergyShieldRegen>(entity, 25.0f);
	context.registry.emplace_or_replace<MaxSpeed>(entity, 40.0f);
	context.registry.emplace_or_replace<KilledScore>(entity, BASE_SCORE * 2);

	weapon::emplaceRandomWeapon(context, entity);
	int subWeapons = GetRandomValue(0, 1000);
	weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {+radius * 1.5f, 0, 0}), subWeapons);
	weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {-radius * 1.5f, 0, 0}), subWeapons);

	context.registry.emplace_or_replace<tag::EliteUnit>(entity);
	return entity;
}

entt::entity spawnFastEliteUnit(GameContext &context, const Vector3& pos) {
	entt::entity entity = spawnBaseUnit(context, pos);

	float radius = 2.0f;
	context.registry.emplace_or_replace<CollisionBody>(entity, radius);
	RenderBody &renderBody = context.registry.get<RenderBody>(entity);
	renderBody.scale = Vector3Ones * radius;
	context.registry.emplace_or_replace<HP>(entity, 720.0f);
	context.registry.emplace_or_replace<HPRegen>(entity, 1.0f);
	context.registry.emplace_or_replace<MaxSpeed>(entity, 160.0f);
	context.registry.emplace_or_replace<TurnSpeed>(entity, 1.5f);
	context.registry.emplace_or_replace<KilledScore>(entity, BASE_SCORE * 2);

	weapon::emplaceRandomWeapon(context, entity);
	int subWeapons = GetRandomValue(0, 1000);
	weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {+radius * 1.5f, 0, 0}), subWeapons);
	weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {-radius * 1.5f, 0, 0}), subWeapons);

	context.registry.emplace_or_replace<tag::EliteUnit>(entity);
	return entity;
}

entt::entity spawnMothershipUnit(GameContext &context, const Vector3& pos) {
	entt::entity entity = spawnBaseUnit(context, pos);

	float radius = 6.0f;
	context.registry.emplace_or_replace<CollisionBody>(entity, radius);
	RenderBody &renderBody = context.registry.get<RenderBody>(entity);
	renderBody.scale = Vector3Ones * radius;
	context.registry.emplace_or_replace<HP>(entity, 4800.0f);
	context.registry.emplace_or_replace<HPRegen>(entity, 50.0f);
	context.registry.emplace_or_replace<EnergyShield>(entity, 1000.0f);
	context.registry.emplace_or_replace<EnergyShieldRegen>(entity, 100.0f);
	context.registry.emplace_or_replace<MaxSpeed>(entity, 20.0f);
	context.registry.emplace_or_replace<TurnSpeed>(entity, 1.25f);
	context.registry.emplace_or_replace<KilledScore>(entity, BASE_SCORE * 5);

	weapon::emplaceRandomWeapon(context, entity);
	int randNum = GetRandomValue(0, 1000);
	weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {+radius * 1.5f, +radius * 0.8f, -2}), randNum);
	weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {+radius * 1.5f, -radius * 0.8f, -2}), randNum);
	weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {-radius * 1.5f, +radius * 0.8f, -2}), randNum);
	weapon::emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {-radius * 1.5f, -radius * 0.8f, -2}), randNum);

	weapon::emplaceWeaponBasic(context, spawnLinkedTurret(context, renderBody.color, entity, {+radius * 2.0f, +radius * 1.2f, -1}));
	weapon::emplaceWeaponBasic(context, spawnLinkedTurret(context, renderBody.color, entity, {+radius * 2.0f, -radius * 1.2f, -1}));
	weapon::emplaceWeaponBasic(context, spawnLinkedTurret(context, renderBody.color, entity, {-radius * 2.0f, +radius * 1.2f, -1}));
	weapon::emplaceWeaponBasic(context, spawnLinkedTurret(context, renderBody.color, entity, {-radius * 2.0f, -radius * 1.2f, -1}));
	context.registry.emplace_or_replace<tag::EliteUnit>(entity);
	return entity;
}