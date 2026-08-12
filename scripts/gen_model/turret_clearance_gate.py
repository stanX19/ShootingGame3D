#!/usr/bin/env python3
"""Independent turret-envelope collision gate for generated spaceship meshes.

The C++ generator performs an in-process clearance check while it builds the
mesh.  This script deliberately re-loads the emitted OBJ and JSON instead of
trusting that implementation.  Every configured turret is sampled through its
full traverse cone and checked as a conservative swept envelope against the
emitted ship mesh.  The front-tip probes remain useful diagnostics, but are not
the firing gate: an obstruction anywhere in the traversable cone fails.
Sparse batteries are also
checked against other turret spheres and nominal forward barrels; dense
capital batteries keep the sphere check but intentionally allow their external
barrel envelopes to interleave, since that is their deliberate visual mode.
The same independent pass also checks the local radius-1 parent collision sphere,
per-mount fixed-facing requests, and left/right vertex symmetry so a malformed
wing or fairing cannot hide behind a successful firing corridor.
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont


EPSILON = 1.0e-8
FAIL_EPSILON = 1.0e-4
UNIT_PARENT_COLLISION_RADIUS = 1.0
UNIT_PARENT_CLEARANCE_MARGIN = 0.02
FIXED_FACING_TOLERANCE_DEGREES = 0.5
SYMMETRY_TOLERANCE = 1.0e-4
AZIMUTH_SAMPLES = 64
SAMPLE_RINGS = (0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1.0)
FIRING_CLEARANCE_MARGIN = 0.02


def load_obj(path: Path) -> np.ndarray:
    positions: list[tuple[float, float, float]] = []
    triangles: list[tuple[int, int, int]] = []
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        parts = raw_line.split()
        if not parts:
            continue
        if parts[0] == "v":
            positions.append(tuple(float(value) for value in parts[1:4]))
        elif parts[0] == "f":
            corners = [int(value.split("/")[0]) - 1 for value in parts[1:]]
            for index in range(1, len(corners) - 1):
                triangles.append((corners[0], corners[index], corners[index + 1]))
    if not positions or not triangles:
        raise ValueError(f"OBJ contains no usable triangles: {path}")
    vertices = np.asarray(positions, dtype=np.float64)
    indices = np.asarray(triangles, dtype=np.int64)
    return vertices[indices]


def load_vertices(path: Path) -> np.ndarray:
    """Load the emitted vertex cloud for the left/right balance audit."""
    positions: list[tuple[float, float, float]] = []
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        parts = raw_line.split()
        if parts and parts[0] == "v":
            positions.append(tuple(float(value) for value in parts[1:4]))
    if not positions:
        raise ValueError(f"OBJ contains no vertices: {path}")
    return np.asarray(positions, dtype=np.float64)


def material_mapping_report(obj_path: Path) -> dict[str, object]:
    """Verify that the emitted OBJ actually carries diffuse and normal maps.

    A rendered mesh can look flat when the generator wrote the images but the OBJ
    lost its UV/normal references or the MTL points at a missing relative file.  Keep
    this independent of the C++ writer so the final QA report proves the material
    contract, not just the silhouette.
    """
    mtl_path = obj_path.with_suffix(".mtl")
    errors: list[str] = []
    map_kd: str | None = None
    bump: str | None = None
    if not mtl_path.exists():
        errors.append(f"missing MTL: {mtl_path}")
    else:
        for raw_line in mtl_path.read_text(encoding="utf-8").splitlines():
            parts = raw_line.split()
            if len(parts) >= 2 and parts[0] == "map_Kd":
                map_kd = " ".join(parts[1:])
            elif len(parts) >= 2 and parts[0] in {"bump", "map_Bump", "map_bump"}:
                bump = " ".join(parts[1:])
    referenced = {}
    for label, relative in (("diffuse", map_kd), ("normal", bump)):
        if relative is None:
            errors.append(f"MTL has no {label} texture reference")
            continue
        texture_path = (mtl_path.parent / relative).resolve()
        referenced[label] = str(texture_path)
        if not texture_path.exists() or texture_path.stat().st_size <= 0:
            errors.append(f"missing {label} texture: {texture_path}")
        else:
            try:
                with Image.open(texture_path) as image:
                    if image.width < 2 or image.height < 2:
                        errors.append(f"{label} texture is not usable: {texture_path}")
            except OSError as error:
                errors.append(f"cannot decode {label} texture {texture_path}: {error}")

    obj_text = obj_path.read_text(encoding="utf-8")
    texcoord_count = sum(1 for line in obj_text.splitlines() if line.startswith("vt "))
    normal_count = sum(1 for line in obj_text.splitlines() if line.startswith("vn "))
    face_lines = [line for line in obj_text.splitlines() if line.startswith("f ")]
    if texcoord_count == 0:
        errors.append("OBJ has no texture coordinates")
    if normal_count == 0:
        errors.append("OBJ has no vertex normals")
    if face_lines and any(any(len(corner.split("/")) < 3 or not corner.split("/")[1] or not corner.split("/")[2]
                             for corner in line.split()[1:]) for line in face_lines):
        errors.append("OBJ faces do not reference both UVs and normals")
    return {
        "mtl": str(mtl_path),
        "diffuse": referenced.get("diffuse"),
        "normal": referenced.get("normal"),
        "texcoordCount": texcoord_count,
        "normalCount": normal_count,
        "pass": not errors,
        "errors": errors,
    }


def symmetry_report(vertices: np.ndarray) -> dict[str, object]:
    """Compare a vertex set with its X-mirrored set within the stated tolerance.

    MeshBuilder duplicates vertices for hard edges and triangle fans, so this is a
    set audit rather than a fragile index/order comparison.  A neighbor-bucket
    search is required here: rounding each coordinate independently can put two
    points only 2e-7 apart on opposite sides of a 1e-4 rounding boundary and
    falsely report an asymmetric ship.
    """
    scale = 1.0 / SYMMETRY_TOLERANCE
    unique_vertices = np.unique(np.round(vertices, decimals=7), axis=0)
    buckets: dict[tuple[int, int, int], list[np.ndarray]] = {}
    for vertex in unique_vertices:
        key = tuple(np.floor(vertex * scale).astype(np.int64).tolist())
        buckets.setdefault(key, []).append(vertex)
    tolerance_squared = SYMMETRY_TOLERANCE * SYMMETRY_TOLERANCE
    unmatched = 0
    for vertex in unique_vertices:
        mirrored = np.array((-vertex[0], vertex[1], vertex[2]), dtype=np.float64)
        cell = np.floor(mirrored * scale).astype(np.int64)
        found = False
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                for dz in (-1, 0, 1):
                    key = (int(cell[0] + dx), int(cell[1] + dy), int(cell[2] + dz))
                    for candidate in buckets.get(key, ()):
                        if float(np.dot(candidate - mirrored, candidate - mirrored)) \
                                <= tolerance_squared:
                            found = True
                            break
                    if found:
                        break
                if found:
                    break
            if found:
                break
        if not found:
            unmatched += 1
    return {
        "tolerance": SYMMETRY_TOLERANCE,
        "vertexCount": int(len(vertices)),
        "uniqueVertexCount": int(len(unique_vertices)),
        "unmatchedMirrorVertices": unmatched,
        "pass": unmatched == 0,
    }


def normalize(vector: np.ndarray) -> np.ndarray:
    length = float(np.linalg.norm(vector))
    if length <= EPSILON:
        raise ValueError("zero-length direction in spaceship configuration")
    return vector / length


def sample_directions(forward: np.ndarray, half_angle_degrees: float) -> list[tuple[np.ndarray, float, float]]:
    """Tessellate the complete traverse cone, including ring-cell boundaries.

    The emitted rays are inflated by ``cone_sampling_inflation`` below.  That
    inflation covers the angular gap between neighboring deterministic samples,
    so this is a conservative cone envelope rather than a front-tip heuristic.
    """
    forward = normalize(forward)
    reference = np.array([0.0, 1.0, 0.0]) if abs(float(forward[1])) < 0.9 else np.array([1.0, 0.0, 0.0])
    right = normalize(np.cross(reference, forward))
    up = normalize(np.cross(forward, right))
    directions: list[tuple[np.ndarray, float, float]] = [(forward, 0.0, 0.0)]
    for fraction in SAMPLE_RINGS:
        angle = math.radians(half_angle_degrees * fraction)
        for sample in range(AZIMUTH_SAMPLES):
            azimuth = 2.0 * math.pi * sample / AZIMUTH_SAMPLES
            radial = right * math.cos(azimuth) + up * math.sin(azimuth)
            directions.append((
                normalize(forward * math.cos(angle) + radial * math.sin(angle)),
                half_angle_degrees * fraction,
                math.degrees(azimuth) % 360.0,
            ))
    return directions


def point_triangle_distance_squared(point: np.ndarray, a: np.ndarray, b: np.ndarray, c: np.ndarray) -> float:
    # Real-Time Collision Detection, closest point on triangle.
    ab = b - a
    ac = c - a
    ap = point - a
    d1 = float(np.dot(ab, ap))
    d2 = float(np.dot(ac, ap))
    if d1 <= 0.0 and d2 <= 0.0:
        return float(np.dot(ap, ap))
    bp = point - b
    d3 = float(np.dot(ab, bp))
    d4 = float(np.dot(ac, bp))
    if d3 >= 0.0 and d4 <= d3:
        return float(np.dot(bp, bp))
    vc = d1 * d4 - d3 * d2
    if vc <= 0.0 and d1 >= 0.0 and d3 <= 0.0:
        amount = d1 / max(d1 - d3, EPSILON)
        delta = point - (a + amount * ab)
        return float(np.dot(delta, delta))
    cp = point - c
    d5 = float(np.dot(ab, cp))
    d6 = float(np.dot(ac, cp))
    if d6 >= 0.0 and d5 <= d6:
        return float(np.dot(cp, cp))
    vb = d5 * d2 - d1 * d6
    if vb <= 0.0 and d2 >= 0.0 and d6 <= 0.0:
        amount = d2 / max(d2 - d6, EPSILON)
        delta = point - (a + amount * ac)
        return float(np.dot(delta, delta))
    va = d3 * d6 - d5 * d4
    if va <= 0.0 and (d4 - d3) >= 0.0 and (d5 - d6) >= 0.0:
        amount = (d4 - d3) / max((d4 - d3) + (d5 - d6), EPSILON)
        delta = point - (b + amount * (c - b))
        return float(np.dot(delta, delta))
    normal = np.cross(ab, ac)
    normal_length = float(np.linalg.norm(normal))
    if normal_length <= EPSILON:
        return min(
            point_segment_distance_squared(point, a, b),
            point_segment_distance_squared(point, a, c),
            point_segment_distance_squared(point, b, c),
        )
    distance = float(np.dot(point - a, normal / normal_length))
    return distance * distance


def point_segment_distance_squared(point: np.ndarray, start: np.ndarray, end: np.ndarray) -> float:
    direction = end - start
    length_squared = float(np.dot(direction, direction))
    if length_squared <= EPSILON:
        delta = point - start
        return float(np.dot(delta, delta))
    amount = max(0.0, min(1.0, float(np.dot(point - start, direction)) / length_squared))
    delta = point - (start + amount * direction)
    return float(np.dot(delta, delta))


def segment_intersects_triangle(start: np.ndarray, end: np.ndarray, a: np.ndarray, b: np.ndarray, c: np.ndarray) -> bool:
    direction = end - start
    edge1 = b - a
    edge2 = c - a
    h = np.cross(direction, edge2)
    determinant = float(np.dot(edge1, h))
    if abs(determinant) <= EPSILON:
        return False
    inverse = 1.0 / determinant
    s = start - a
    u = inverse * float(np.dot(s, h))
    if u < -EPSILON or u > 1.0 + EPSILON:
        return False
    q = np.cross(s, edge1)
    v = inverse * float(np.dot(direction, q))
    if v < -EPSILON or u + v > 1.0 + EPSILON:
        return False
    amount = inverse * float(np.dot(edge2, q))
    return -EPSILON <= amount <= 1.0 + EPSILON


def segment_segment_distance_squared(
    start_a: np.ndarray,
    end_a: np.ndarray,
    start_b: np.ndarray,
    end_b: np.ndarray,
) -> float:
    direction_a = end_a - start_a
    direction_b = end_b - start_b
    offset = start_a - start_b
    length_a = float(np.dot(direction_a, direction_a))
    length_b = float(np.dot(direction_b, direction_b))
    cross_term = float(np.dot(direction_a, direction_b))
    offset_a = float(np.dot(direction_a, offset))
    offset_b = float(np.dot(direction_b, offset))
    denominator = length_a * length_b - cross_term * cross_term
    if denominator <= EPSILON:
        amount_a = 0.0
        amount_b = max(0.0, min(1.0, offset_b / length_b)) if length_b > EPSILON else 0.0
    else:
        amount_a = max(0.0, min(1.0, (cross_term * offset_b - offset_a * length_b) / denominator))
        amount_b = (cross_term * amount_a + offset_b) / length_b if length_b > EPSILON else 0.0
        if amount_b < 0.0:
            amount_b = 0.0
            amount_a = max(0.0, min(1.0, -offset_a / length_a)) if length_a > EPSILON else 0.0
        elif amount_b > 1.0:
            amount_b = 1.0
            amount_a = max(0.0, min(1.0, (cross_term - offset_a) / length_a)) if length_a > EPSILON else 0.0
    delta = (start_a + amount_a * direction_a) - (start_b + amount_b * direction_b)
    return float(np.dot(delta, delta))


def segment_triangle_distance_squared(start: np.ndarray, end: np.ndarray, triangle: np.ndarray) -> float:
    a, b, c = triangle
    if segment_intersects_triangle(start, end, a, b, c):
        return 0.0
    return min(
        point_triangle_distance_squared(start, a, b, c),
        point_triangle_distance_squared(end, a, b, c),
        segment_segment_distance_squared(start, end, a, b),
        segment_segment_distance_squared(start, end, b, c),
        segment_segment_distance_squared(start, end, c, a),
    )


def candidate_triangles(triangles: np.ndarray, lower: np.ndarray, upper: np.ndarray) -> np.ndarray:
    triangle_lower = triangles.min(axis=1)
    triangle_upper = triangles.max(axis=1)
    mask = np.all(triangle_upper >= lower, axis=1) & np.all(triangle_lower <= upper, axis=1)
    return np.flatnonzero(mask)


def capsule_mesh_clearance(
    triangles: np.ndarray,
    start: np.ndarray,
    end: np.ndarray,
    radius: float,
) -> tuple[float, int | None]:
    lower = np.minimum(start, end) - radius
    upper = np.maximum(start, end) + radius
    best = math.inf
    best_triangle: int | None = None
    for triangle_index in candidate_triangles(triangles, lower, upper):
        distance = math.sqrt(segment_triangle_distance_squared(start, end, triangles[triangle_index])) - radius
        if distance < best:
            best = distance
            best_triangle = int(triangle_index)
    return best, best_triangle


def point_mesh_clearance(
    triangles: np.ndarray,
    point: np.ndarray,
    radius: float,
) -> tuple[float, int | None]:
    lower = point - radius
    upper = point + radius
    best = math.inf
    best_triangle: int | None = None
    for triangle_index in candidate_triangles(triangles, lower, upper):
        distance = math.sqrt(point_triangle_distance_squared(point, *triangles[triangle_index])) - radius
        if distance < best:
            best = distance
            best_triangle = int(triangle_index)
    return best, best_triangle


def mount_capsule(mount: dict, direction: np.ndarray) -> tuple[np.ndarray, np.ndarray, float]:
    position = np.asarray([mount["position"][axis] for axis in ("x", "y", "z")], dtype=np.float64)
    direction = normalize(direction)
    radius = float(mount["barrelRadius"])
    # Match the generator's physical barrel envelope: the first two barrel
    # radii are the protected neck inside the turret sphere, while the separate
    # front-tip probes below verify the long exposed cylinder corridor.
    start = position + direction * (float(mount["turretRadius"]) + radius * 2.0)
    end = position + direction * float(mount["barrelLength"])
    return start, end, radius


def cone_sampling_inflation(mount: dict) -> float:
    """Small numerical skin for the dense complete-cone tessellation."""
    return max(0.001, float(mount["barrelLength"]) * 0.0005)


def peer_clearance(
    moving_position: np.ndarray,
    moving_turret_radius: float,
    moving_start: np.ndarray,
    moving_end: np.ndarray,
    moving_radius: float,
    mount_index: int,
    mounts: list[dict],
    check_peer_barrels: bool,
) -> tuple[float, str | None, int | None]:
    best = math.inf
    best_peer: str | None = None
    best_peer_index: int | None = None
    for peer_index, peer in enumerate(mounts):
        if peer_index == mount_index:
            continue
        peer_position = np.asarray([peer["position"][axis] for axis in ("x", "y", "z")], dtype=np.float64)
        sphere_clearance = float(np.linalg.norm(moving_position - peer_position)) \
            - moving_turret_radius - float(peer["turretRadius"])
        if sphere_clearance < best:
            best = sphere_clearance
            best_peer = str(peer["id"])
            best_peer_index = peer_index
        peer_turret_clearance = math.sqrt(point_segment_distance_squared(peer_position, moving_start, moving_end)) \
            - moving_radius - float(peer["turretRadius"])
        if peer_turret_clearance < best:
            best = peer_turret_clearance
            best_peer = str(peer["id"])
            best_peer_index = peer_index
        if check_peer_barrels:
            peer_direction = np.asarray([peer["forward"][axis] for axis in ("x", "y", "z")], dtype=np.float64)
            peer_start, peer_end, peer_radius = mount_capsule(peer, peer_direction)
            barrel_clearance = math.sqrt(segment_segment_distance_squared(
                moving_start, moving_end, peer_start, peer_end
            )) - moving_radius - peer_radius
            if barrel_clearance < best:
                best = barrel_clearance
                best_peer = str(peer["id"])
                best_peer_index = peer_index
    return best, best_peer, best_peer_index


def camera_basis(name: str) -> tuple[np.ndarray, np.ndarray]:
    if name == "top":
        return np.array([1.0, 0.0, 0.0]), np.array([0.0, 0.0, -1.0])
    if name == "side":
        return np.array([0.0, 0.0, 1.0]), np.array([0.0, 1.0, 0.0])
    if name == "front":
        return np.array([1.0, 0.0, 0.0]), np.array([0.0, 1.0, 0.0])
    azimuth = math.radians(45.0)
    elevation = math.radians(25.0)
    direction = np.array([
        math.sin(azimuth) * math.cos(elevation),
        math.sin(elevation),
        math.cos(azimuth) * math.cos(elevation),
    ])
    right = normalize(np.array([direction[2], 0.0, -direction[0]]))
    return right, normalize(np.cross(direction, right))


def write_debug_screenshot(
    path: Path,
    triangles: np.ndarray,
    mount: dict,
    failure: dict,
    mounts: list[dict],
) -> None:
    """Render the failed capsule, tip, and offending triangle for diagnosis."""
    image_size = 1024
    panel_size = image_size // 2
    image = Image.new("RGB", (image_size, image_size), (8, 14, 22))
    draw = ImageDraw.Draw(image)
    font = ImageFont.load_default()
    all_points = triangles.reshape(-1, 3)
    all_points = np.vstack((all_points, failure["start"], failure["end"], failure["tip75"]))
    lower = all_points.min(axis=0)
    upper = all_points.max(axis=0)
    center = (lower + upper) * 0.5
    extent = max(float(np.max(upper - lower)), 1.0)
    panel_padding = 28.0
    scale = (panel_size - panel_padding * 2.0) / extent
    offending = failure.get("meshTriangle")
    peer_id = failure.get("peer")
    peer_index = failure.get("peerIndex")
    if isinstance(peer_index, int) and 0 <= peer_index < len(mounts):
        peer_mount = mounts[peer_index]
    else:
        peer_mount = next((candidate for candidate in mounts if candidate["id"] == peer_id), None)
    peer_position = None
    peer_start = None
    peer_end = None
    peer_sphere_radius = 0.0
    if peer_mount is not None:
        peer_position = np.asarray([peer_mount["position"][axis] for axis in ("x", "y", "z")], dtype=np.float64)
        peer_direction = np.asarray([peer_mount["forward"][axis] for axis in ("x", "y", "z")], dtype=np.float64)
        peer_start, peer_end, _ = mount_capsule(peer_mount, peer_direction)
        peer_sphere_radius = float(peer_mount["turretRadius"])

    def project(point: np.ndarray, right: np.ndarray, up: np.ndarray, origin: tuple[int, int]) -> tuple[float, float]:
        relative = point - center
        return (
            origin[0] + panel_size * 0.5 + float(np.dot(relative, right)) * scale,
            origin[1] + panel_size * 0.5 - float(np.dot(relative, up)) * scale,
        )

    panels = (("top", (0, 0)), ("side", (panel_size, 0)), ("front", (0, panel_size)), ("three-quarter", (panel_size, panel_size)))
    for name, origin in panels:
        right, up = camera_basis(name if name != "three-quarter" else "three-quarter")
        for triangle_index, triangle in enumerate(triangles):
            polygon = [project(vertex, right, up, origin) for vertex in triangle]
            fill = (80, 92, 104)
            outline = (38, 48, 58)
            if triangle_index == offending:
                fill = (190, 38, 88)
                outline = (255, 148, 182)
            draw.polygon(polygon, fill=fill, outline=outline)
        start = project(failure["start"], right, up, origin)
        end = project(failure["end"], right, up, origin)
        tip = project(failure["tip75"], right, up, origin)
        draw.line((start, end), fill=(244, 62, 54), width=5)
        draw.ellipse((tip[0] - 6, tip[1] - 6, tip[0] + 6, tip[1] + 6), fill=(255, 222, 54), outline=(255, 255, 220))
        if peer_position is not None and peer_start is not None and peer_end is not None:
            peer_screen = project(peer_position, right, up, origin)
            peer_pixel_radius = max(3.0, peer_sphere_radius * scale)
            draw.ellipse((
                peer_screen[0] - peer_pixel_radius,
                peer_screen[1] - peer_pixel_radius,
                peer_screen[0] + peer_pixel_radius,
                peer_screen[1] + peer_pixel_radius,
            ), fill=(220, 130, 42), outline=(255, 220, 130), width=2)
            draw.line((project(peer_start, right, up, origin), project(peer_end, right, up, origin)), fill=(220, 130, 42), width=3)
        draw.text((origin[0] + 12, origin[1] + 10), name.upper(), fill=(235, 242, 247), font=font)

    angle_text = f"{failure['angleDegrees']:.2f} deg cone / {failure['azimuthDegrees']:.2f} deg az"
    coordinate_text = "pos=(" + ", ".join(f"{value:.3f}" for value in failure["mountPosition"]) + ")"
    detail_text = f"{mount['id']}  {failure['failureType']}  clearance={failure['clearance']:.4f}"
    draw.rectangle((12, image_size - 58, image_size - 12, image_size - 12), fill=(5, 9, 15))
    draw.text((22, image_size - 51), detail_text, fill=(255, 170, 170), font=font)
    draw.text((22, image_size - 36), angle_text + "  " + coordinate_text, fill=(235, 242, 247), font=font)
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path)


def latest_obj(output_root: Path, ship_id: str) -> Path:
    candidates = list(output_root.glob(f"{ship_id}_*/spaceship_{ship_id}.obj"))
    if not candidates:
        raise FileNotFoundError(f"no generated OBJ found for {ship_id} under {output_root}")
    return max(candidates, key=lambda path: path.stat().st_mtime_ns)


def json_safe(value: object) -> object:
    """Keep the diagnostic report valid JSON instead of emitting Infinity/NaN."""
    if isinstance(value, float) and not math.isfinite(value):
        return None
    if isinstance(value, dict):
        return {key: json_safe(item) for key, item in value.items()}
    if isinstance(value, list):
        return [json_safe(item) for item in value]
    return value


def merged_mounts(catalog: dict, ship: dict, obj_path: Path) -> list[dict]:
    defaults = dict(catalog.get("defaults", {}))
    report_path = obj_path.parent / "generation_report.json"
    if report_path.exists():
        report = json.loads(report_path.read_text(encoding="utf-8"))
        resolved = report.get("resolvedMounts")
        if isinstance(resolved, list) and resolved:
            mounts = []
            for mount in resolved:
                merged = dict(defaults)
                merged.update(mount)
                mounts.append(merged)
            return mounts
    mounts = []
    for mount in ship.get("mounts", []):
        merged = dict(defaults)
        merged.update(mount)
        mounts.append(merged)
    # Match spaceship_config.cpp: the generator applies the per-deck cant after
    # parsing mount metadata, so the independent gate must audit the same
    # effective forward vectors that the emitted model was built against.
    cant_degrees = float(ship.get("layout", {}).get("weaponDeckCantDegrees", 0.0))
    if abs(cant_degrees) > EPSILON:
        cant_radians = math.radians(cant_degrees)
        cosine = math.cos(cant_radians)
        sine = math.sin(cant_radians)
        for mount in mounts:
            forward = mount["forward"]
            horizontal_length = math.hypot(float(forward["x"]), float(forward["z"]))
            if horizontal_length <= EPSILON:
                raise ValueError("weapon mount cannot cant a vertical forward vector")
            deck_side = -1.0 if float(mount["position"]["y"]) < 0.0 else 1.0
            mount["forward"] = {
                "x": float(forward["x"]) / horizontal_length * cosine,
                "y": deck_side * sine,
                "z": float(forward["z"]) / horizontal_length * cosine,
            }
    return mounts


def normalized_facing(mount: dict) -> np.ndarray | None:
    raw = mount.get("facingDirection", {"x": 0.0, "y": 0.0, "z": 1.0})
    if raw is None:
        return None
    vector = np.asarray([float(raw[axis]) for axis in ("x", "y", "z")], dtype=np.float64)
    magnitude = float(np.linalg.norm(vector))
    if not math.isfinite(magnitude) or magnitude <= EPSILON:
        raise ValueError(f"invalid facingDirection for mount {mount.get('id', '<unknown>')}")
    return vector / magnitude


def failure_record(
    mount: dict,
    position: np.ndarray,
    direction: np.ndarray,
    clearance: float,
    failure_type: str,
) -> dict:
    start, end, _ = mount_capsule(mount, direction)
    tip75 = position + direction * (float(mount["barrelLength"]) * 0.75)
    return {
        "clearance": clearance,
        "failureType": failure_type,
        "meshTriangle": None,
        "peer": None,
        "peerIndex": None,
        "start": start,
        "end": end,
        "tip75": tip75,
        "tip100": end,
        "mountPosition": position.copy(),
        "angleDegrees": 0.0,
        "azimuthDegrees": 0.0,
        "direction": [float(value) for value in direction],
    }


def check_ship(ship_id: str, ship: dict, catalog: dict, output_root: Path) -> dict:
    obj_path = latest_obj(output_root, ship_id)
    triangles = load_obj(obj_path)
    symmetry = symmetry_report(load_vertices(obj_path))
    materials = material_mapping_report(obj_path)
    mounts = merged_mounts(catalog, ship, obj_path)
    mount_results: list[dict] = []
    peer_barrel_check = len(mounts) < 6
    for mount_index, mount in enumerate(mounts):
        position = np.asarray([mount["position"][axis] for axis in ("x", "y", "z")], dtype=np.float64)
        effective_forward = np.asarray(
            [mount["forward"][axis] for axis in ("x", "y", "z")], dtype=np.float64
        )
        effective_length = float(np.linalg.norm(effective_forward))
        if not math.isfinite(effective_length) or effective_length <= EPSILON:
            raise ValueError(f"invalid resolved forward for {ship_id}/{mount['id']}")
        effective_forward /= effective_length
        requested = normalized_facing(mount)
        facing_error = 0.0
        if requested is not None:
            facing_error = math.degrees(
                math.acos(float(np.clip(np.dot(requested, effective_forward), -1.0, 1.0)))
            )
        parent_clearance = float(np.linalg.norm(position)) - UNIT_PARENT_COLLISION_RADIUS - float(mount["turretRadius"])
        worst_gate_failure: dict | None = None
        if parent_clearance - UNIT_PARENT_CLEARANCE_MARGIN < 0.0:
            worst_gate_failure = failure_record(
                mount,
                position,
                effective_forward,
                parent_clearance - UNIT_PARENT_CLEARANCE_MARGIN,
                "parentCollisionSphere",
            )
        if requested is not None and FIXED_FACING_TOLERANCE_DEGREES - facing_error < 0.0:
            facing_failure = failure_record(
                mount,
                position,
                effective_forward,
                FIXED_FACING_TOLERANCE_DEGREES - facing_error,
                "fixedFacingDirection",
            )
            if worst_gate_failure is None or facing_failure["clearance"] < worst_gate_failure["clearance"]:
                worst_gate_failure = facing_failure
        directions = sample_directions(
            effective_forward,
            float(mount["traverseHalfAngleDegrees"]),
        )
        mesh_min = math.inf
        mesh_triangle: int | None = None
        peer_min = math.inf
        peer_id: str | None = None
        peer_index: int | None = None
        tip75_min = math.inf
        tip100_min = math.inf
        worst_direction: list[float] | None = None
        worst_failure: dict | None = None
        for direction, angle_degrees, azimuth_degrees in directions:
            start, end, radius = mount_capsule(mount, direction)
            # Cover the angular cell between neighboring deterministic rays.  A
            # plain ray lattice can miss a thin rib between samples; the inflated
            # capsule turns the lattice into a conservative complete-cone gate.
            cone_radius = radius + cone_sampling_inflation(mount)
            mesh_clearance, triangle_index = capsule_mesh_clearance(
                triangles, start, end, cone_radius
            )
            peer_clearance_value, peer, peer_mount_index = peer_clearance(
                position,
                float(mount["turretRadius"]),
                start,
                end,
                radius,
                mount_index,
                mounts,
                peer_barrel_check,
            )
            tip75 = position + direction * (float(mount["barrelLength"]) * 0.75)
            tip100 = end
            tip75_clearance, tip75_triangle = point_mesh_clearance(triangles, tip75, radius)
            tip100_clearance, tip100_triangle = point_mesh_clearance(triangles, tip100, radius)
            if mesh_clearance < mesh_min:
                mesh_min = mesh_clearance
                mesh_triangle = triangle_index
                worst_direction = [float(value) for value in direction]
            if peer_clearance_value < peer_min:
                peer_min = peer_clearance_value
                peer_id = peer
                peer_index = peer_mount_index
            tip75_min = min(tip75_min, tip75_clearance)
            tip100_min = min(tip100_min, tip100_clearance)
            samples = (
                (mesh_clearance, "meshCapsule", triangle_index, peer),
                (peer_clearance_value, "peerEnvelope", None, peer),
                (tip75_clearance, "frontTip75", tip75_triangle, peer),
                (tip100_clearance, "frontTip100", tip100_triangle, peer),
            )
            gate_samples = [samples[0], samples[2], samples[3]]
            if peer_barrel_check:
                gate_samples.append(samples[1])
            for clearance, failure_type, failure_triangle, failure_peer in samples:
                if worst_failure is None or clearance < worst_failure["clearance"]:
                    worst_failure = {
                        "clearance": clearance,
                        "failureType": failure_type,
                        "meshTriangle": failure_triangle,
                        "peer": failure_peer,
                        "peerIndex": peer_mount_index,
                        "start": start.copy(),
                        "end": end.copy(),
                        "tip75": tip75.copy(),
                        "tip100": tip100.copy(),
                        "mountPosition": position.copy(),
                        "angleDegrees": angle_degrees,
                        "azimuthDegrees": azimuth_degrees,
                        "direction": [float(value) for value in direction],
                    }
            for clearance, failure_type, failure_triangle, failure_peer in gate_samples:
                if worst_gate_failure is None or clearance < worst_gate_failure["clearance"]:
                    worst_gate_failure = {
                        "clearance": clearance,
                        "failureType": failure_type,
                        "meshTriangle": failure_triangle,
                        "peer": failure_peer,
                        "peerIndex": peer_mount_index,
                        "start": start.copy(),
                        "end": end.copy(),
                        "tip75": tip75.copy(),
                        "tip100": tip100.copy(),
                        "mountPosition": position.copy(),
                        "angleDegrees": angle_degrees,
                        "azimuthDegrees": azimuth_degrees,
                        "direction": [float(value) for value in direction],
                    }
        # Sparse batteries must never sweep through a neighbour's barrel or
        # turret sphere.  Dense capital batteries keep the peer value as a
        # diagnostic, but their many independently steered external barrels
        # are intentionally not a hard gate.  The complete firing cone is.
        gate_clearance = min(
            mesh_min - FIRING_CLEARANCE_MARGIN,
            parent_clearance - UNIT_PARENT_CLEARANCE_MARGIN,
        )
        if requested is not None:
            gate_clearance = min(
                gate_clearance,
                FIXED_FACING_TOLERANCE_DEGREES - facing_error,
            )
        if peer_barrel_check:
            gate_clearance = min(gate_clearance, peer_min)
        failed = gate_clearance < -FAIL_EPSILON
        debug_screenshot: str | None = None
        if failed and worst_gate_failure is not None:
            screenshot_path = output_root / "turret-clearance-failures" / f"{ship_id}_{mount_index:02d}_{mount['id']}.png"
            write_debug_screenshot(screenshot_path, triangles, mount, worst_gate_failure, mounts)
            debug_screenshot = str(screenshot_path)
        mount_results.append({
            "id": mount["id"],
            "mountIndex": mount_index,
            "requestedFacing": None if requested is None else [float(value) for value in requested],
            "effectiveForward": [float(value) for value in effective_forward],
            "facingErrorDegrees": facing_error,
            "parentCollisionClearance": parent_clearance,
            "traverseHalfAngleDegrees": mount["traverseHalfAngleDegrees"],
            "meshMinimumClearance": mesh_min,
            "meshTriangle": mesh_triangle,
            "peerMinimumClearance": peer_min,
            "peer": peer_id,
            "frontTip75MinimumClearance": tip75_min,
            "frontTip100MinimumClearance": tip100_min,
            "worstDirection": worst_direction,
            "worstFailure": None if worst_gate_failure is None else {
                key: value.tolist() if isinstance(value, np.ndarray) else value
                for key, value in worst_gate_failure.items()
            },
            "worstDiagnosticFailure": None if worst_failure is None else {
                key: value.tolist() if isinstance(value, np.ndarray) else value
                for key, value in worst_failure.items()
            },
            "debugScreenshot": debug_screenshot,
            "pass": not failed,
        })
    return {
        "id": ship_id,
        "obj": str(obj_path),
        "triangleCount": int(len(triangles)),
        "symmetry": symmetry,
        "materials": materials,
        "peerCheckMode": "sphere-and-barrel-gate" if peer_barrel_check else "sphere-diagnostic-only-dense-battery",
        "mounts": mount_results,
        "pass": all(result["pass"] for result in mount_results),
    }


def main() -> int:
    workspace = Path(__file__).resolve().parents[3]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path, default=workspace / "project/assets/config/spaceships.json")
    parser.add_argument("--output-root", type=Path, default=workspace / "scratch/model-qc/spaceships")
    parser.add_argument("--report", type=Path, default=None)
    args = parser.parse_args()
    config_path = args.config.resolve()
    output_root = args.output_root.resolve()
    catalog = json.loads(config_path.read_text(encoding="utf-8"))
    report = {
        "config": str(config_path),
        "outputRoot": str(output_root),
        "sampleRings": list(SAMPLE_RINGS),
        "azimuthSamples": AZIMUTH_SAMPLES,
        "frontTipFraction": 0.75,
        "firingClearanceMargin": FIRING_CLEARANCE_MARGIN,
        "coneAzimuthSamples": AZIMUTH_SAMPLES,
        "coneRings": list(SAMPLE_RINGS),
        "clearanceNullMeaning": "No triangle AABB intersects the swept capsule/point broad phase; this is a clear positive-space result, not missing data.",
        "hardGate": "unit parent sphere + fixed facing + conservative complete emitted-OBJ traverse-cone envelope; sparse peer sphere/barrel envelopes",
        "socketPolicy": "The independent OBJ gate includes bearing/socket triangles as physical geometry; this is intentionally stricter than the generator's tagged-socket exemption.",
        "ships": [],
    }
    failures: list[str] = []
    for ship_id, ship in catalog.get("ships", {}).items():
        try:
            result = check_ship(ship_id, ship, catalog, output_root)
        except (OSError, ValueError, KeyError) as error:
            failures.append(f"{ship_id}: {error}")
            continue
        report["ships"].append(result)
        if not result["symmetry"]["pass"]:
            failures.append(
                f"{ship_id}: left/right symmetry failed with "
                f"{result['symmetry']['unmatchedMirrorVertices']} unmatched vertices "
                f"at tolerance {result['symmetry']['tolerance']}"
            )
        if not result["materials"]["pass"]:
            failures.append(f"{ship_id}: material mapping failed: {result['materials']['errors']}")
        for mount in result["mounts"]:
            if not mount["pass"]:
                failures.append(
                    f"{ship_id}/{mount['id']}: mesh={mount['meshMinimumClearance']:.6f}, "
                    f"peer={mount['peerMinimumClearance']:.6f} ({mount['peer']}), "
                    f"tip75={mount['frontTip75MinimumClearance']:.6f}, "
                    f"tip100={mount['frontTip100MinimumClearance']:.6f}, "
                    f"parent={mount['parentCollisionClearance']:.6f}, "
                    f"facingError={mount['facingErrorDegrees']:.3f}, "
                    f"failure={mount['worstFailure']}, screenshot={mount['debugScreenshot']}"
                )
    report_path = args.report.resolve() if args.report else output_root / "turret_clearance_gate.json"
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(json_safe(report), indent=2) + "\n", encoding="utf-8")
    if failures:
        print("turret_clearance_gate: FAIL")
        for failure in failures:
            print(f"  {failure}")
        return 1
    print(f"turret_clearance_gate: PASS ({len(report['ships'])} ships, independent OBJ complete-cone gate)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
