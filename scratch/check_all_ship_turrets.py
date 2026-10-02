import glob
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

# Load turret
turret_verts, turret_tris = load_obj('assets/Models/turrets/basic_shooter/turret_basic_shooter.obj')

import json

for report_path in sorted(glob.glob("assets/Models/spaceships/*/generation_report.json")):
    ship_dir = report_path.rsplit('/', 1)[0]
    ship_id = ship_dir.split('/')[-1]
    obj_path = f"{ship_dir}/spaceship_{ship_id}.obj"
    with open(report_path) as fp:
        data = json.load(fp)
    mounts = data.get("resolvedMounts", [])
    if not mounts:
        continue
    ship_verts, ship_tris = load_obj(obj_path)
    
    # Check each mount
    for m in mounts:
        m_pos = np.array([m["position"][axis] for axis in ("x", "y", "z")])
        m_rad = float(m["turretRadius"])
        
        # Check distance of all ship vertices to mount position
        dists = np.linalg.norm(ship_verts - m_pos, axis=1)
        inside = np.where(dists < m_rad)[0]
        if len(inside) > 0:
            min_dist = np.min(dists)
            print(f"SHIP [{ship_id}] mount [{m['id']}]: {len(inside)} verts inside turret sphere! min_dist={min_dist:.4f} < {m_rad}")
