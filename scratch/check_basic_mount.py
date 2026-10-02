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
    return np.array(verts), np.array(tris)

ship_verts, ship_tris = load_obj('assets/Models/spaceships/basic/spaceship_basic.obj')
turret_verts, turret_tris = load_obj('assets/Models/turrets/basic_shooter/turret_basic_shooter.obj')

# mount_0: pos={'x': 1.4500000476837158, 'y': 0.4966300427913666, 'z': -0.23616136610507965}, rad=0.25
mount_pos = np.array([1.4500000476837158, 0.4966300427913666, -0.23616136610507965])
turret_scaled = turret_verts * 0.25 + mount_pos

t_min = np.min(turret_scaled, axis=0)
t_max = np.max(turret_scaled, axis=0)
print(f"Turret bounds at mount_0: min={t_min}, max={t_max}")

in_box = np.all((ship_verts >= t_min) & (ship_verts <= t_max), axis=1)
in_box_indices = np.where(in_box)[0]
print(f"Ship vertices inside turret bounding box: {len(in_box_indices)}")
for idx in in_box_indices:
    print(f"  Ship vertex {idx+1}: {ship_verts[idx]}")

dists_to_center = np.linalg.norm(ship_verts - mount_pos, axis=1)
inside_sphere = np.where(dists_to_center < 0.25)[0]
print(f"Ship vertices strictly inside turret pivot sphere: {len(inside_sphere)}")
for idx in inside_sphere:
    print(f"  v[{idx+1}]: {ship_verts[idx]} dist={dists_to_center[idx]:.4f}")
