# Model Asset Conventions

These conventions apply to runtime 3D models and to offline model generators.

## Nominal unit radius

Runtime model loading assumes that a model is authored around a nominal local radius of `1.0`. `RenderBody.scale` is the world-space nominal radius; a model is not corrected or normalized on the loading path.

Irregular asteroid geometry has the approved local-radius tolerance `0.95–1.05`. Its `CollisionBody.radius` is therefore a spherical approximation of the nominal radius and may differ from individual protrusions or depressions by at most 5%.

Models should be centered at their intended local origin. Any asset-specific normalization or geometry generation belongs in the offline asset pipeline under `scripts/gen_model/`, not in entity factories or the runtime renderer.

## Ownership

- Offline generators own procedural geometry, UVs, normals, and texture generation.
- `ModelManager` owns runtime file loading, material texture preparation, caching, and unloading.
- Entity factories request model IDs and compose runtime components. They do not walk model materials or mutate GPU texture state.
- The renderer consumes the prepared `Model` and does not contain asset-specific loading policy.

## OBJ/MTL texture references

OBJ geometry references its MTL with `mtllib`. The MTL references external image files such as `map_Kd` for diffuse color and `bump` for tangent-space normal data. These files must remain together in the model directory. Mipmaps and texture filtering are runtime GPU state prepared by `ModelManager`; they are not embedded in OBJ or MTL files.
