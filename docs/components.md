# ECS Component Domain Reference

> [!IMPORTANT]
> **Maintenance Contract:**
> - If any discrepancy between this document and the active codebase is spotted, you **must raise it to the user immediately** to align and update this document.
> - Whenever adding, modifying, renaming, or removing ECS components, domains, or tags in the codebase, you **must update this document** in tandem as part of the same change.

---

## 1. Component Domains Overview
Every ECS component belongs to a cohesive domain header under `headers/components/<domain>.hpp` scoped inside `namespace <domain>`.

| Domain Header | Namespace | Core Components (`domain::Component`) | Tags (`domain::tag::TagName`) | Responsibility |
|---|---|---|---|---|
| `physics.hpp` | `physics` | `Position`, `Velocity`, `ScalarAcceleration`, `Rotation`, `PrevRotation`, `RotationVelocity`, `Mass`, `ImpulseRequest` | `VelocitySyncRot` | Pure Newtonian kinematic & physical state. |
| `identity.hpp` | `identity` | `Name` | `Spaceship`, `Asteroid` | Entity identification and macro archetype classification. |
| `spaceship.hpp` | `spaceship` | `MaxSpeed`, `TurnSpeed`, `TargetVelocity`, `TargetRotation`, `MoveTarget` | `AIMoveControl`, `Suicidal` | Vehicle flight envelope and steering control targets. |
| `combat.hpp` | `combat` | `HP`, `HPRegen`, `EnergyShield`, `EnergyShieldRegen`, `Damage`, `DelayedDamage`, `DamageContributors` | `Targetable` | Vitality, health regeneration, and damage payloads. |
| `weapon.hpp` | `weapon` | `Weapon`, `Ammo`, `AmmoRegen`, `AmmoReload`, `AimDirection`, `AimTarget`, `ChargedWeapon`, `ExtendFireDuration`, `ExtendFireRequest`, `JustFired`, `WeaponCooldown`, `WeaponName`, `WeaponParent` | `IsWeapon`, `AIControlledAim`, `PlayerControlledFire`, `AIControlledFire`, `Bullet`, `Missile`, `Kinetic`, `Energy`, `Lazer`, etc. | Weapon state, aiming, ammunition, and projectile tags. |
| `render.hpp` | `render` | `RenderBody`, `ModelStrech`, `RadiusExpand` | `LightSource`, `Shaded`, `SkyBox`, `RotationSyncModel`, `AimDirectionSyncModel`, `VelocitySyncModelRot` | Visual meshes, rendering transforms, and model sync tags. |
| `collision.hpp` | `collision` | `CollisionBody`, `CollisionBodyModel` | *(none)* | Collision spheres and mesh proxy references. |
| `anchor.hpp` | `anchor` | `PositionAnchor`, `RotationAnchor`, `DeathAnchor` | `GetVelOnAnchorDeath` | Parent-relative attachment and hierarchy. |
| `score.hpp` | `score` | `Score`, `ScoreParent`, `KilledScore` | *(none)* | Scoring progression, attribution, and kill bounties. |
| `lifetime.hpp` | `lifetime` | `Lifespan`, `DisappearBound` | *(none)* | Temporal duration and spatial arena boundary cleanup. |
| `effect.hpp` | `effect` | `ExplodeOnDeath`, `InstantDamageOnDeath`, `HasSimpleTrail`, `HasMultiTrail`, `Trail` | `DropDebris` | Visual FX, debris, procedural ribbon trails, and death explosions. |
| `faction.hpp` | `faction` | `Faction`, `FacVal`, constants (`FAC_NONE`, `FAC_BLUE`, `FAC_RED`, `FAC_BULLET`, `FAC_ASTEROID`) | *(none)* | Faction allegiances and friend-or-foe queries. |
| `camera.hpp` | `camera` | `UnitCamera`, `CameraPOV`, `emplaceUnitCameraBasic()` | *(none)* | Camera POV configuration and camera tracking state. |
| `sound.hpp` | `sound` | `HitSound`, `ShootSound`, `DeathSound` | *(none)* | Sound FX event triggers for weapons, hits, and deaths. |

---

## 2. Component Design Principles

### Single-Field Payload Uniformity
Single-field wrapper components must name their payload member `.value` (e.g. `Position::value`, `Velocity::value`, `HP::value`, `Name::value`). Never introduce ad-hoc abbreviations such as `.val`. This naming consistency enables generic and reflective utilities to inspect and manipulate components uniformly without special-casing.

### Behavioral Tag Semantics (No Vanity Tags)
- Tags must strictly represent **mechanical capabilities or behavioral contracts** within systems (e.g. `tag::AIMoveControl`, `tag::Suicidal`, `tag::Targetable`).
- Never introduce tags for marketing labels, difficulties, or balance tiers (e.g. `tag::EliteUnit`). Ship tier distinctions belong in data/JSON configs (`assets/config/units.json`), not in the ECS component signature.

### Spawner Dispatch Tables
When selecting or dispatching between multiple entity archetypes (such as unit respawn systems), prefer a static `std::to_array` dispatch table of function pointers:
```cpp
using ShipSpawner = entt::entity (*)(GameContext&, const Vector3&, faction::Faction);
static constexpr auto kShipSpawners = std::to_array<ShipSpawner>({
    spawnUnit,
    spawnFighterUnit,
    spawnEliteUnit,
    spawnFastEliteUnit,
    spawnTerminatorUnit,
    spawnMothershipUnit,
});
```
This ensures uniform $O(1)$ random selection and effortless extension without branching `switch-case` ladders.

### Tag Namespace Isolation
Each domain header provides backwards-compatibility aliases in `namespace tag { using ::domain::tag::TagName; }`.
Translation units (`.cpp` files) must **never** use blanket `using namespace <domain>;` when multiple included domains declare an inner `tag` namespace, as this causes compiler ambiguity errors (`reference to 'tag' is ambiguous`). Instead, qualify domain components directly (`physics::Position`, `weapon::tag::Bullet`).
