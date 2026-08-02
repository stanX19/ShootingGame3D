# Model Asset Conventions

These conventions apply to runtime 3D models and to offline model generators.

## Nominal unit radius

Runtime model loading assumes that a model is authored around a nominal local radius of `1.0`. `RenderBody.scale` is the world-space nominal radius; a model is not corrected or normalized on the loading path.

Asteroid visual geometry uses the approved local-radius range `0.8–1.0`. Its `CollisionBody.radius` remains the nominal radius `1.0`, so the sphere is a conservative broad-phase proxy: the visual model never extends beyond the initial collision radius. Future mesh-collision refinement may reject broad-phase false positives without changing this asset convention.

Models should be centered at their intended local origin. Any asset-specific normalization or geometry generation belongs in the offline asset pipeline under `scripts/gen_model/`, not in entity factories or the runtime renderer.

## Ownership

- Offline generators own procedural geometry, UVs, normals, and texture generation.
- `ModelManager` owns runtime file loading, material texture preparation, caching, and unloading.
- Entity factories request model IDs and compose runtime components. They do not walk model materials or mutate GPU texture state.
- The renderer consumes the prepared `Model` and does not contain asset-specific loading policy.

## OBJ/MTL texture references

OBJ geometry references its MTL with `mtllib`. The MTL references external image files such as `map_Kd` for diffuse color and `bump` for tangent-space normal data. These files must remain together in the model directory. Mipmaps and texture filtering are runtime GPU state prepared by `ModelManager`; they are not embedded in OBJ or MTL files.
