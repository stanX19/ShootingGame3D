import numpy as np

def load_obj(path):
    verts, tris = [], []
    with open(path) as f:
        for line in f:
            if line.startswith('v '):
                verts.append([float(x) for x in line.split()[1:4]])
            elif line.startswith('f '):
                corners = [int(p.split('/')[0]) - 1 for p in line.split()[1:4]]
                tris.append(corners)
    return np.array(verts, dtype=np.float64), np.array(tris, dtype=np.int64)

for arch in ['basic_shooter', 'heavy_mg_rifle', 'lazer_deletor', 'lazer_shotgun']:
    path = f'assets/Models/turrets/{arch}/turret_{arch}.obj'
    verts, tris = load_obj(path)
    print(f"\n=== {arch} ===")
    print(f"Total verts: {len(verts)}, tris: {len(tris)}")
    
    # Sphere vertices are the first 165 vertices (11 * 15)
    sphere_verts = verts[:165]
    other_verts = verts[165:]
    
    dists = np.linalg.norm(other_verts, axis=1)
    print(f"Min dist of other verts to origin: {np.min(dists):.6f}")
    print(f"Number of other verts with dist < 1.00001: {np.sum(dists < 1.00001)}")
    print(f"Number of other verts with dist < 1.01: {np.sum(dists < 1.01)}")
    print(f"Number of other verts with dist < 1.05: {np.sum(dists < 1.05)}")
    
    # Tri-tri self-intersection test
    from test_ship_turret_intersect import tri_tri_intersect
    
    tri_pts = verts[tris]
    tri_mins = np.min(tri_pts, axis=1)
    tri_maxs = np.max(tri_pts, axis=1)
    
    self_intersects = []
    for i in range(len(tris)):
        for j in range(i + 1, len(tris)):
            # Skip triangles sharing vertices
            if len(set(tris[i]).intersection(set(tris[j]))) > 0:
                continue
            if np.any(tri_mins[i] > tri_maxs[j]) or np.any(tri_maxs[i] < tri_mins[j]):
                continue
            if tri_tri_intersect(tri_pts[i][0], tri_pts[i][1], tri_pts[i][2],
                                tri_pts[j][0], tri_pts[j][1], tri_pts[j][2]):
                self_intersects.append((i, j))
    print(f"Self-intersecting non-adjacent triangle pairs: {len(self_intersects)}")
    for i, j in self_intersects[:10]:
        print(f"  Tri {i} intersects Tri {j}")

