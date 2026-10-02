import numpy as np

def load_obj(path):
    verts = []
    tris = []
    with open(path) as f:
        for line in f:
            if line.startswith('v '):
                verts.append([float(x) for x in line.split()[1:4]])
            elif line.startswith('f '):
                corners = [int(p.split('/')[0]) - 1 for p in line.split()[1:4]]
                tris.append(corners)
    return np.array(verts, dtype=np.float64), np.array(tris, dtype=np.int64)

ship_verts, ship_tris = load_obj('assets/Models/spaceships/player/spaceship_player.obj')
turret_verts, turret_tris = load_obj('assets/Models/turrets/basic_shooter/turret_basic_shooter.obj')

mount_pos = np.array([1.6700000762939453, 0.7234958410263062, 0.17061734199523926])
turret_scaled = turret_verts * 0.25 + mount_pos

# Möller triangle-triangle intersection in 3D
def tri_tri_intersect(v0, v1, v2, u0, u1, u2, eps=1e-5):
    n2 = np.cross(u1 - u0, u2 - u0)
    len_n2 = np.linalg.norm(n2)
    if len_n2 < 1e-9: return False
    n2 = n2 / len_n2
    d2 = -np.dot(n2, u0)

    du0 = np.dot(n2, v0) + d2
    du1 = np.dot(n2, v1) + d2
    du2 = np.dot(n2, v2) + d2

    if (du0 >= -eps and du1 >= -eps and du2 >= -eps) or (du0 <= eps and du1 <= eps and du2 <= eps):
        return False
    if abs(du0) <= eps and abs(du1) <= eps and abs(du2) <= eps:
        return False

    n1 = np.cross(v1 - v0, v2 - v0)
    len_n1 = np.linalg.norm(n1)
    if len_n1 < 1e-9: return False
    n1 = n1 / len_n1
    d1 = -np.dot(n1, v0)

    dv0 = np.dot(n1, u0) + d1
    dv1 = np.dot(n1, u1) + d1
    dv2 = np.dot(n1, u2) + d1

    if (dv0 >= -eps and dv1 >= -eps and dv2 >= -eps) or (dv0 <= eps and dv1 <= eps and dv2 <= eps):
        return False
    if abs(dv0) <= eps and abs(dv1) <= eps and abs(dv2) <= eps:
        return False

    d = np.cross(n1, n2)
    max_d = np.argmax(np.abs(d))
    if np.abs(d[max_d]) < 1e-9: return False

    def get_interval(vv0, vv1, vv2, d0, d1, d2):
        if d0 * d1 > 0:
            p0, p1, p2 = vv0[max_d], vv1[max_d], vv2[max_d]
            dp0, dp1, dp2 = d0, d1, d2
        elif d0 * d2 > 0:
            p0, p1, p2 = vv0[max_d], vv2[max_d], vv1[max_d]
            dp0, dp1, dp2 = d0, d2, d1
        else:
            p0, p1, p2 = vv1[max_d], vv2[max_d], vv0[max_d]
            dp0, dp1, dp2 = d1, d2, d0
        den0 = dp2 - dp0
        den1 = dp2 - dp1
        t0 = p2 + (p0 - p2) * (dp2 / den0) if abs(den0) > 1e-9 else p2
        t1 = p2 + (p1 - p2) * (dp2 / den1) if abs(den1) > 1e-9 else p2
        if t0 > t1: t0, t1 = t1, t0
        return t0, t1

    t0_min, t0_max = get_interval(v0, v1, v2, du0, du1, du2)
    t1_min, t1_max = get_interval(u0, u1, u2, dv0, dv1, dv2)
    overlap = min(t0_max, t1_max) - max(t0_min, t1_min)
    return overlap > eps

# Filter candidate triangles by bounding box
t_min = np.min(turret_scaled, axis=0) - 0.01
t_max = np.max(turret_scaled, axis=0) + 0.01

ship_tri_pts = ship_verts[ship_tris]
tri_mins = np.min(ship_tri_pts, axis=1)
tri_maxs = np.max(ship_tri_pts, axis=1)

candidate_ship_tris = np.where(
    np.all(tri_maxs >= t_min, axis=1) & np.all(tri_mins <= t_max, axis=1)
)[0]

print(f"Candidate ship triangles near turret: {len(candidate_ship_tris)}")

intersections = []
for s_idx in candidate_ship_tris:
    sv0, sv1, sv2 = ship_tri_pts[s_idx]
    s_min = np.min([sv0, sv1, sv2], axis=0)
    s_max = np.max([sv0, sv1, sv2], axis=0)
    for t_idx, (tv0, tv1, tv2) in enumerate(turret_scaled[turret_tris]):
        t_tri_min = np.min([tv0, tv1, tv2], axis=0)
        t_tri_max = np.max([tv0, tv1, tv2], axis=0)
        if np.any(s_min > t_tri_max) or np.any(s_max < t_tri_min):
            continue
        if tri_tri_intersect(sv0, sv1, sv2, tv0, tv1, tv2):
            intersections.append((s_idx, t_idx))

print(f"Total ship-turret intersecting triangle pairs: {len(intersections)}")
for s_idx, t_idx in intersections[:20]:
    print(f"  Ship Tri {s_idx} intersects Turret Tri {t_idx}")

if intersections:
    s_idx, t_idx = intersections[0]
    print(f"Turret Tri {t_idx}:")
    for p in turret_scaled[turret_tris[t_idx]]:
        print(" ", p)
    print(f"Ship Tri {s_idx}:")
    for p in ship_verts[ship_tris[s_idx]]:
        print(" ", p)
