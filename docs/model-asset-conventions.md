# Model Asset Conventions

These conventions apply to runtime 3D models and to offline model generators.

## Nominal unit radius (MANDATORY ENFORCING RULE)

Every 3D model in this game (spaceships, asteroids, turrets, weapons, props) MUST be authored around a nominal local base radius of exactly `1.0f` centered at `(0, 0, 0)`.

`RenderBody.scale` is the world-space nominal radius multiplier; a model is not corrected or normalized on the runtime loading path.

### Specific Turret Contract
- Center `(0, 0, 0)` MUST be the exact center of the pivot sphere.
- The pivot sphere radius MUST be exactly `1.0f`.
- This guarantees that when the game scales the turret by `mount.turretRadius` (e.g. 0.25f), the rendered sphere radius is exactly `1.0f * turretRadius = turretRadius`, perfectly resting in the spaceship socket blister without hovering or clipping.

### Mandatory Verification Evidence Rule
After every single model generation work, the developer or agent MUST run geometric inspection on the generated OBJ model and show the user verifiable evidence that:
1. The pivot / local origin is centered at `(0.0, 0.0, 0.0)`.
2. The nominal base radius is verified to be exactly `1.0f`.
3. The model extents match the expected unit dimensions.

Models should be centered at their intended local origin. Any asset-specific normalization or geometry generation belongs in the offline asset pipeline under `scripts/gen_model/`, not in entity factories or the runtime renderer.

The generated spaceship family uses the same local-unit contract for every
profile. The parent collision proxy is authored at radius `1.0`; its OBJ
geometry, resolved engine coordinates, and resolved mount coordinates share
those local units. `spaceship::factory` uniformly multiplies the model, engines,
mounts, turret dimensions, and `CollisionBody.radius` by the caller's requested
positive radius. `runtime.modelRadius` records the generated mesh bound for
inspection and is not a normalization factor. This permits wings, engines,
radiators, and weapon structures to extend beyond the gameplay collision sphere
while allowing callers to choose any desired world size without moving turrets
back toward the origin.

Automatic weapon mounts are authored as indexed capabilities. A missing
`facingDirection` is fixed local front (`+Z`); `null` delegates direction to the
coverage solver and is currently reserved for Terminator's distributed battery.
Both generator and serialized-OBJ gates sweep the complete configured traverse
cone (64 azimuth samples over eight radial rings plus the center) with a
positive clearance margin. A 75% barrel-tip probe is diagnostic only.

Propulsion geometry is not a universal nozzle prefab. The selected propulsion
layout determines the rear manifold, collar profile, shroud, service fairing,
and tapered aft pressure volume around the resolved nozzle cells. This keeps a
fighter's paired nacelles, a spine cluster, and a capital side-block bank
visibly distinct while preserving the same unit-radius runtime contract.

## Procedural Primitive Models (`default:<primitive>`)

Weapons and visual props can specify procedural geometric primitives in configuration via the `default:<primitive>` URI scheme instead of external 3D file paths:
- `default:sphere`: GenMeshSphere with nominal radius `1.0f`.
- `default:cube` (or `default:box`): GenMeshCube with side length `2.0f` spanning `[-1.0, 1.0]`.
- `default:cylinder`: GenMeshCylinder with radius `1.0f` and height `2.0f`.
- `default:plane`: GenMeshPlane with width `2.0f` and length `2.0f`.

All procedural primitives conform strictly to the mandatory nominal unit radius of `1.0f` centered at `(0, 0, 0)`. The game engine scales them uniformly via `RenderBody.scale` and projectile `radius`.

## Collision proxy conventions

A `.collision.obj` is a CPU-side physics/query asset, not a render LOD. It may be much simpler than the visual model and should contain only surfaces that matter to gameplay. Its use is explicit: a caller loads the collision model and adds `CollisionBodyModel` to the entity. The presence of a companion file alone does not opt an entity into mesh-based collision.

`CollisionBodyManager` caches the immutable proxy triangles, bounds, static model transform, and BVH. Runtime entity translation, scale, and current rotation are supplied by the entity's `RenderBody` transform contract when the proxy is queried. Do not bake an instance transform into shared asset data, and do not apply a static model transform twice.

The sphere in `CollisionBody` remains the conservative broad-phase representation. The detector derives a transient effective radius from the authored sphere and the transformed proxy bound; it does not rewrite the component's radius. Because rotation and scale can change the furthest transformed vertex, only immutable model data is globally cacheable.

If a proxy is used for containment, it must be authored as a closed, consistently oriented, sufficiently manifold volume. A genuine hole, such as a donut opening, must remain empty; filling holes or assuming convexity would create false positives. Open or non-manifold proxies require an explicit surface-only policy or validation failure.

## Ownership

- Offline generators own procedural geometry, UVs, normals, and texture generation.
- `ModelManager` owns runtime file loading, material texture preparation, caching, and unloading.
- Entity factories request model IDs and compose runtime components. They do not walk model materials or mutate GPU texture state.
- The renderer consumes the prepared `Model` and does not contain asset-specific loading policy.

## OBJ/MTL texture references

OBJ geometry references its MTL with `mtllib`. The MTL references external image files such as `map_Kd` for diffuse color and `bump` for tangent-space normal data. These files must remain together in the model directory. Mipmaps and texture filtering are runtime GPU state prepared by `ModelManager`; they are not embedded in OBJ or MTL files.
