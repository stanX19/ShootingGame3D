import numpy as np

def load_obj(path):
    verts = []
    tris = []
    with open(path) as f:
        for line in f:
            if line.startswith('v '):
                verts.append([float(x) for x in line.split()[1:4]])
            elif line.startswith('f '):
                # f v1/vt1/vn1 v2/vt2/vn2 ...
                corners = [int(p.split('/')[0]) - 1 for p in line.split()[1:4]]
                tris.append(corners)
    return np.array(verts), np.array(tris)

ship_verts, ship_tris = load_obj('assets/Models/spaceships/player/spaceship_player.obj')
turret_verts, turret_tris = load_obj('assets/Models/turrets/basic_shooter/turret_basic_shooter.obj')

# Scale and translate turret to mount_0:
# mount_0: position = [1.6700000762939453, 0.7234958410263062, 0.17061734199523926]
# turretRadius = 0.25
mount_pos = np.array([1.6700000762939453, 0.7234958410263062, 0.17061734199523926])
turret_scaled = turret_verts * 0.25 + mount_pos

# Find bounding box of turret
t_min = np.min(turret_scaled, axis=0)
t_max = np.max(turret_scaled, axis=0)

print(f"Turret bounds at mount_0: min={t_min}, max={t_max}")

# Find ship vertices inside turret bounding box
in_box = np.all((ship_verts >= t_min) & (ship_verts <= t_max), axis=1)
in_box_indices = np.where(in_box)[0]
print(f"Ship vertices inside turret bounding box: {len(in_box_indices)}")
for idx in in_box_indices:
    print(f"  Ship vertex {idx+1}: {ship_verts[idx]}")

# Find ship triangles that intersect turret sphere (radius 0.25 at mount_pos)
ship_tri_pts = ship_verts[ship_tris]
# Check distance from mount_pos to triangle
dists_to_center = np.linalg.norm(ship_verts - mount_pos, axis=1)
inside_sphere = np.where(dists_to_center < 0.25)[0]
print(f"Ship vertices strictly inside turret pivot sphere: {len(inside_sphere)}")
for idx in inside_sphere:
    print(f"  v[{idx+1}]: {ship_verts[idx]} dist={dists_to_center[idx]:.4f}")
