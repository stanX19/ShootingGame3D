#include "entities.hpp"
#include "weapons.hpp"
#include "weapon_registry.hpp"
#include "components/sound.hpp"
#include "factions.hpp"
#include "utils.hpp"

entt::entity spawnBaseUnit(GameContext &context, const Vector3& pos, float radius, faction::Faction faction) {
	entt::entity entity = context.registry.create();
	
	const GameConfig& cfg = context.config;
	const float baseHp = cfg.getFloat("units.base.hp", 1000.0f);
	const float baseShield = cfg.getFloat("units.base.shield", 500.0f);
	const float shieldRegenDiv = cfg.getFloat("units.base.shieldRegenDivisor", 15.0f);
	const float baseSpeed = cfg.getFloat("units.base.speed", 80.0f);
	const float baseTurn = cfg.getFloat("units.base.turnSpeed", 1.5f);
	const float baseDamage = cfg.getFloat("units.base.damage", 500.0f);
	const int baseScore = cfg.getInt("units.base.score", 500);

	t_model_id shipModel = context.modelManager.loadModel("assets/Models/spacechip1/model/Intergalactic_Spaceship-(Wavefront).obj", 0.4f);
	context.registry.emplace<Position>(entity, pos);
	context.registry.emplace<Velocity>(entity);
	context.registry.emplace<Rotation>(entity);
	context.registry.emplace<CollisionBody>(entity, radius);
	context.registry.emplace<RenderBody>(entity, RenderBody{
		shipModel, WHITE, Vector3Ones * radius
	});
	context.registry.emplace<HP>(entity, baseHp);
	context.registry.emplace<EnergyShield>(entity, baseShield);
	context.registry.emplace<EnergyShieldRegen>(entity, baseShield / shieldRegenDiv);
	context.registry.emplace<Damage>(entity, baseDamage);
	context.registry.emplace<MaxSpeed>(entity, baseSpeed);
	context.registry.emplace<TurnSpeed>(entity, baseTurn);
	context.registry.emplace<Score>(entity);
	context.registry.emplace<KilledScore>(entity, baseScore);
	context.registry.emplace<MoveTarget>(entity);
	context.registry.emplace<Mass>(entity, cfg.getFloat("units.base.mass", 1000.0f));
	
	context.registry.emplace<tag::Targetable>(entity);
	context.registry.emplace<tag::Spaceship>(entity);
	context.registry.emplace<tag::Shaded>(entity);
	context.registry.emplace<tag::RotationSyncModel>(entity);
	context.registry.emplace<tag::effect::DropDebris>(entity);
	context.registry.emplace<tag::effect::ExplodeOnDeath>(entity);
	context.registry.emplace_or_replace<sound::DeathSound>(entity, sound::RANDOM_EXPLOSION, 0.5f * radius);

	context.registry.emplace<tag::weapon::AIControlledAim>(entity);
	context.registry.emplace<tag::weapon::AIControlledFire>(entity);
	context.registry.emplace<tag::weapon::IsSpecialWeapon>(entity);
	context.registry.emplace<tag::AIMoveControl>(entity);
	context.registry.emplace<faction::Faction>(entity, faction);
	return entity;
}

entt::entity spawnUnit(GameContext &context, const Vector3& pos, faction::Faction faction) {
	const GameConfig& cfg = context.config;
	const float radius = cfg.getFloat("units.basic.radius", 1.0f);
	entt::entity entity = spawnBaseUnit(context, pos, radius, faction);

	weapon::emplaceWeaponMissileBasic(context, entity, context.config.getSubConfig("weapons.missile.weapons.basic"));
	RenderBody &renderBody = context.registry.get<RenderBody>(entity);
	int subWeapons = GetRandomValue(0, 1000);
	context.weaponRegistry.emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {+radius * 1.5f, 0, 0}), subWeapons);
	context.weaponRegistry.emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {-radius * 1.5f, 0, 0}), subWeapons);
	return entity;
}

entt::entity spawnEliteUnit(GameContext &context, const Vector3& pos, faction::Faction faction) {
	const GameConfig& cfg = context.config;
	const float radius = cfg.getFloat("units.elite.radius", 3.0f);

	entt::entity entity = spawnBaseUnit(context, pos, radius, faction);

	const float baseSpeed = cfg.getFloat("units.base.speed", 80.0f);
	const int baseScore = cfg.getInt("units.base.score", 500);

	RenderBody &renderBody = context.registry.get<RenderBody>(entity);
	context.registry.emplace_or_replace<HP>(entity, cfg.getFloat("units.elite.hp", 1200.0f));
	context.registry.emplace_or_replace<HPRegen>(entity, cfg.getFloat("units.elite.hpRegen", 10.0f));
	context.registry.emplace_or_replace<EnergyShield>(entity, cfg.getFloat("units.elite.shield", 1000.0f));
	context.registry.emplace_or_replace<EnergyShieldRegen>(entity, cfg.getFloat("units.elite.shieldRegen", 25.0f));
	context.registry.emplace_or_replace<MaxSpeed>(entity, baseSpeed * cfg.getFloat("units.elite.speedMultiplier", 0.5f));
	context.registry.emplace_or_replace<KilledScore>(entity, baseScore * cfg.getInt("units.elite.scoreMultiplier", 2));
	context.registry.emplace_or_replace<Mass>(entity, cfg.getFloat("units.elite.mass", 1000.0f));

	context.weaponRegistry.emplaceRandomSpecialWeapon(context, entity);
	int subWeapons = GetRandomValue(0, 1000);
	context.weaponRegistry.emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {+radius * 1.5f, 0, 0}), subWeapons);
	context.weaponRegistry.emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {-radius * 1.5f, 0, 0}), subWeapons);

	context.registry.emplace_or_replace<tag::EliteUnit>(entity);
	return entity;
}

entt::entity spawnFastEliteUnit(GameContext &context, const Vector3& pos, faction::Faction faction) {
	const GameConfig& cfg = context.config;
	const float radius = cfg.getFloat("units.fastElite.radius", 2.0f);

	entt::entity entity = spawnBaseUnit(context, pos, radius, faction);

	const float baseSpeed = cfg.getFloat("units.base.speed", 80.0f);
	const int baseScore = cfg.getInt("units.base.score", 500);

	RenderBody &renderBody = context.registry.get<RenderBody>(entity);
	context.registry.emplace_or_replace<HP>(entity, cfg.getFloat("units.fastElite.hp", 720.0f));
	context.registry.emplace_or_replace<HPRegen>(entity, cfg.getFloat("units.fastElite.hpRegen", 1.0f));
	context.registry.emplace_or_replace<MaxSpeed>(entity, baseSpeed * cfg.getFloat("units.fastElite.speedMultiplier", 2.0f));
	context.registry.emplace_or_replace<KilledScore>(entity, baseScore * cfg.getInt("units.fastElite.scoreMultiplier", 2));
	context.registry.emplace_or_replace<Mass>(entity, cfg.getFloat("units.fastElite.mass", 1000.0f));

	context.weaponRegistry.emplaceRandomSpecialWeapon(context, entity);
	int subWeapons = GetRandomValue(0, 1000);
	context.weaponRegistry.emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {+radius * 1.5f, 0, 0}), subWeapons);
	context.weaponRegistry.emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {-radius * 1.5f, 0, 0}), subWeapons);

	context.registry.emplace_or_replace<tag::EliteUnit>(entity);
	return entity;
}

entt::entity spawnTerminatorUnit(GameContext &context, const Vector3& pos, faction::Faction faction) {
	const float baseSpeed = context.config.getFloat("units.base.speed", 80.0f);
	const float baseTurn = context.config.getFloat("units.base.turnSpeed", 1.5f);
	const int baseScore = context.config.getInt("units.base.score", 500);

	const GameConfig& cfg = context.config.getSubConfig("units.terminator");
	const float radius = cfg.getFloat("radius", 2.0f);

	entt::entity entity = spawnBaseUnit(context, pos, radius, faction);

	RenderBody &renderBody = context.registry.get<RenderBody>(entity);
	renderBody.modelID = context.modelManager.createSphere(32, 32);
	context.registry.emplace_or_replace<HP>(entity, cfg.getFloat("hp", 3200.0f));
	context.registry.emplace_or_replace<HPRegen>(entity, cfg.getFloat("hpRegen", 50.0f));
	context.registry.emplace_or_replace<EnergyShield>(entity, cfg.getFloat("shield", 1500.0f));
	context.registry.emplace_or_replace<EnergyShieldRegen>(entity, cfg.getFloat("shieldRegen", 100.0f));
	context.registry.emplace_or_replace<MaxSpeed>(entity, baseSpeed * cfg.getFloat("speedMultiplier", 1.5f));
	context.registry.emplace_or_replace<TurnSpeed>(entity, baseTurn * cfg.getFloat("turnSpeedMultiplier", 2.5f));
	context.registry.emplace_or_replace<KilledScore>(entity, baseScore * cfg.getInt("scoreMultiplier", 50));
	context.registry.emplace_or_replace<Mass>(entity, cfg.getFloat("mass", 10000.0f));

	int randNum = GetRandomValue(0, 1000);
	// spawn in all directions since it's a terminator
	const int directions = 8;
	const int heightLevels = 4;
	for (int h = 0; h < heightLevels; h++) {
		for (int i = 0; i < directions; i++) {
			Vector3 direction = Vector3Normalize({
				std::cos(i * PI * 2 / directions),
				std::sin(i * PI * 2 / directions),
				h - (heightLevels - 1) / 2.0f
			});
			entt::entity turret = spawnLinkedAutoTurret(
				context,
				renderBody.color,
				entity,
				direction * radius * 1.5f
			);
			RotationAnchor &rotAnchor = context.registry.get_or_emplace<RotationAnchor>(turret);
			rotAnchor.relrot = vector3ToRotation(direction);
			context.weaponRegistry.emplaceRandomWeapon(context, turret, randNum);
		}
	}
	context.registry.emplace_or_replace<tag::EliteUnit>(entity);
	return entity;
}

entt::entity spawnMothershipUnit(GameContext &context, const Vector3& pos, faction::Faction faction) {
	const GameConfig& cfg = context.config;
	const float radius = cfg.getFloat("units.mothership.radius", 6.0f);

	entt::entity entity = spawnBaseUnit(context, pos, radius, faction);

	const float baseSpeed = cfg.getFloat("units.base.speed", 80.0f);
	const float baseTurn = cfg.getFloat("units.base.turnSpeed", 1.5f);
	const int baseScore = cfg.getInt("units.base.score", 500);

	RenderBody &renderBody = context.registry.get<RenderBody>(entity);
	context.registry.emplace_or_replace<HP>(entity, cfg.getFloat("units.mothership.hp", 4800.0f));
	context.registry.emplace_or_replace<HPRegen>(entity, cfg.getFloat("units.mothership.hpRegen", 50.0f));
	context.registry.emplace_or_replace<EnergyShield>(entity, cfg.getFloat("units.mothership.shield", 1000.0f));
	context.registry.emplace_or_replace<EnergyShieldRegen>(entity, cfg.getFloat("units.mothership.shieldRegen", 100.0f));
	context.registry.emplace_or_replace<MaxSpeed>(entity, baseSpeed * cfg.getFloat("units.mothership.speedMultiplier", 0.25f));
	context.registry.emplace_or_replace<TurnSpeed>(entity, baseTurn * cfg.getFloat("units.mothership.turnSpeedMultiplier", 0.5f));
	context.registry.emplace_or_replace<KilledScore>(entity, baseScore * cfg.getInt("units.mothership.scoreMultiplier", 5));
	context.registry.emplace_or_replace<Mass>(entity, cfg.getFloat("units.mothership.mass", 5000.0f));

	weapon::emplaceWeaponMissileFlares(context, entity, context.config.getSubConfig("weapons.missile.weapons.flares"));
	int randNum = GetRandomValue(0, 1000);
	context.weaponRegistry.emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {+radius * 1.5f, +radius * 0.8f, -2}), randNum);
	context.weaponRegistry.emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {+radius * 1.5f, -radius * 0.8f, -2}), randNum);
	context.weaponRegistry.emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {-radius * 1.5f, +radius * 0.8f, -2}), randNum);
	context.weaponRegistry.emplaceRandomWeapon(context, spawnLinkedTurret(context, renderBody.color, entity, {-radius * 1.5f, -radius * 0.8f, -2}), randNum);

	weapon::emplaceWeaponBasic(context, spawnLinkedTurret(context, renderBody.color, entity, {+radius * 2.0f, +radius * 1.2f, -1}), context.config.getSubConfig("weapons.bullet.weapons.basic"));
	weapon::emplaceWeaponBasic(context, spawnLinkedTurret(context, renderBody.color, entity, {+radius * 2.0f, -radius * 1.2f, -1}), context.config.getSubConfig("weapons.bullet.weapons.basic"));
	weapon::emplaceWeaponBasic(context, spawnLinkedTurret(context, renderBody.color, entity, {-radius * 2.0f, +radius * 1.2f, -1}), context.config.getSubConfig("weapons.bullet.weapons.basic"));
	weapon::emplaceWeaponBasic(context, spawnLinkedTurret(context, renderBody.color, entity, {-radius * 2.0f, -radius * 1.2f, -1}), context.config.getSubConfig("weapons.bullet.weapons.basic"));
	context.registry.emplace_or_replace<tag::EliteUnit>(entity);
	return entity;
}