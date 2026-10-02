import numpy as np
from collections import defaultdict

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

# Segment-to-segment distance squared
def seg_seg_dist_sq(p1, p2, p3, p4):
    u = p2 - p1
    v = p4 - p3
    w = p1 - p3
    a = np.dot(u, u)
    b = np.dot(u, v)
    c = np.dot(v, v)
    d = np.dot(u, w)
    e = np.dot(v, w)
    D = a * c - b * b
    sc, sN, sD = D, D, D
    tc, tN, tD = D, D, D
    if D < 1e-8:
        sN = 0.0
        sD = 1.0
        tN = e
        tD = c
    else:
        sN = (b * e - c * d)
        tN = (a * e - b * d)
        if sN < 0.0:
            sN = 0.0
            tN = e
            tD = c
        elif sN > sD:
            sN = sD
            tN = e + b
            tD = c
    if tN < 0.0:
        tN = 0.0
        if -d < 0.0:
            sN = 0.0
        elif -d > a:
            sN = sD
        else:
            sN = -d
            sD = a
    elif tN > tD:
        tN = tD
        if (-d + b) < 0.0:
            sN = 0.0
        elif (-d + b) > a:
            sN = sD
        else:
            sN = (-d + b)
            sD = a
    sc = 0.0 if abs(sN) < 1e-8 else sN / sD
    tc = 0.0 if abs(tN) < 1e-8 else tN / tD
    dP = w + (sc * u) - (tc * v)
    return np.dot(dP, dP)

def pt_tri_dist_sq(p, a, b, c):
    ab = b - a
    ac = c - a
    ap = p - a
    d1 = np.dot(ab, ap)
    d2 = np.dot(ac, ap)
    if d1 <= 0.0 and d2 <= 0.0: return np.dot(p - a, p - a)
    bp = p - b
    d3 = np.dot(ab, bp)
    d4 = np.dot(ac, bp)
    if d3 >= 0.0 and d4 <= d3: return np.dot(p - b, p - b)
    vc = d1 * d4 - d3 * d2
    if vc <= 0.0 and d1 >= 0.0 and d3 <= 0.0:
        v = d1 / (d1 - d3)
        return np.dot(p - (a + v * ab), p - (a + v * ab))
    cp = p - c
    d5 = np.dot(ab, cp)
    d6 = np.dot(ac, cp)
    if d6 >= 0.0 and d5 <= d6: return np.dot(p - c, p - c)
    vb = d5 * d2 - d1 * d6
    if vb <= 0.0 and d2 >= 0.0 and d6 <= 0.0:
        w = d2 / (d2 - d6)
        return np.dot(p - (a + w * ac), p - (a + w * ac))
    va = d3 * d6 - d5 * d4
    if va <= 0.0 and (d4 - d3) >= 0.0 and (d5 - d6) >= 0.0:
        w = (d4 - d3) / ((d4 - d3) + (d5 - d6))
        return np.dot(p - (b + w * (c - b)), p - (b + w * (c - b)))
    denom = 1.0 / (va + vb + vc)
    v = vb * denom
    w = vc * denom
    proj = a + ab * v + ac * w
    return np.dot(p - proj, p - proj)

def tri_tri_dist_sq(t1, t2):
    d2_min = min(pt_tri_dist_sq(t1[0], t2[0], t2[1], t2[2]),
                 pt_tri_dist_sq(t1[1], t2[0], t2[1], t2[2]),
                 pt_tri_dist_sq(t1[2], t2[0], t2[1], t2[2]),
                 pt_tri_dist_sq(t2[0], t1[0], t1[1], t1[2]),
                 pt_tri_dist_sq(t2[1], t1[0], t1[1], t1[2]),
                 pt_tri_dist_sq(t2[2], t1[0], t1[1], t1[2]))
    for i in range(3):
        e1_a, e1_b = t1[i], t1[(i+1)%3]
        for j in range(3):
            e2_a, e2_b = t2[j], t2[(j+1)%3]
            d2_min = min(d2_min, seg_seg_dist_sq(e1_a, e1_b, e2_a, e2_b))
    return d2_min

for arch in ['basic_shooter', 'heavy_mg_rifle', 'lazer_deletor', 'lazer_shotgun']:
    path = f'assets/Models/turrets/{arch}/turret_{arch}.obj'
    verts, tris = load_obj(path)
    
    # Quantize vertices to find connected components by shared 3D positions
    quant = np.round(verts * 10000).astype(np.int64)
    pos_to_id = {}
    vert_to_pos_id = []
    for q in quant:
        t = tuple(q)
        if t not in pos_to_id:
            pos_to_id[t] = len(pos_to_id)
        vert_to_pos_id.append(pos_to_id[t])
    
    # Union-find to find connected components of triangles
    parent = list(range(len(tris)))
    def find(i):
        if parent[i] == i: return i
        parent[i] = find(parent[i])
        return parent[i]
    def union(i, j):
        pi, pj = find(i), find(j)
        if pi != pj: parent[pi] = pj

    pos_to_tris = defaultdict(list)
    for t_idx, tri in enumerate(tris):
        for v in tri:
            pid = vert_to_pos_id[v]
            pos_to_tris[pid].append(t_idx)

    for pid, t_list in pos_to_tris.items():
        for k in range(1, len(t_list)):
            union(t_list[0], t_list[k])

    comp_map = {}
    for t_idx in range(len(tris)):
        root = find(t_idx)
        if root not in comp_map:
            comp_map[root] = len(comp_map)

    tri_comp = np.array([comp_map[find(t_idx)] for t_idx in range(len(tris))])
    num_comps = len(comp_map)
    print(f"\n=== {arch} ===")
    print(f"Total triangles: {len(tris)}, distinct connected components: {num_comps}")

    tri_pts = verts[tris]
    tri_mins = np.min(tri_pts, axis=1)
    tri_maxs = np.max(tri_pts, axis=1)

    threshold = 0.05
    threshold_sq = threshold * threshold
    close_pairs = []
    min_dist_found = 999.0

    for i in range(len(tris)):
        for j in range(i + 1, len(tris)):
            if tri_comp[i] == tri_comp[j]:
                continue
            if np.any(tri_mins[i] - threshold > tri_maxs[j]) or np.any(tri_maxs[i] + threshold < tri_mins[j]):
                continue
            d2 = tri_tri_dist_sq(tri_pts[i], tri_pts[j])
            dist = np.sqrt(d2)
            if dist < min_dist_found:
                min_dist_found = dist
            if d2 < threshold_sq:
                close_pairs.append((i, j, dist))

    print(f"Min clearance between distinct components: {min_dist_found:.6f}")
    print(f"Pairs between distinct components closer than {threshold}: {len(close_pairs)}")
    if close_pairs:
        for i, j, d in close_pairs[:5]:
            print(f"  Tri {i} (comp {tri_comp[i]}) and Tri {j} (comp {tri_comp[j]}): dist = {d:.6f}")
