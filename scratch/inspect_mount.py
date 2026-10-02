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

# Test both player and interceptor_quad
for ship_name in ['player', 'interceptor_quad']:
    print(f"\n=================== SHIP: {ship_name} ===================")
    import json
    rep_path = f'assets/Models/spaceships/{ship_name}/generation_report.json'
    with open(rep_path) as f:
        rep = json.load(f)
    
    mounts = rep.get('resolvedMounts', rep.get('mounts', []))
    ship_verts, ship_tris = load_obj(f'assets/Models/spaceships/{ship_name}/spaceship_{ship_name}.obj')
    turret_verts, turret_tris = load_obj('assets/Models/turrets/basic_shooter/turret_basic_shooter.obj')
    
    from test_ship_turret_intersect import tri_tri_intersect
    
    for m_idx, mount in enumerate(mounts):
        pos_dict = mount['position']
        m_pos = np.array([pos_dict['x'], pos_dict['y'], pos_dict['z']])
        m_rad = float(mount.get('turretRadius', 0.25))
        print(f"\nMount {m_idx} ({mount.get('id', '')}): pos={m_pos}, rad={m_rad}")
        
        # Check ship vertices near mount center
        dists = np.linalg.norm(ship_verts - m_pos, axis=1)
        inside_verts = np.where(dists < m_rad)[0]
        print(f"  Ship vertices STRICTLY INSIDE turret sphere (dist < {m_rad}): {len(inside_verts)}")
        for vi in inside_verts[:5]:
            print(f"    v {vi}: {ship_verts[vi]}, dist={dists[vi]:.5f} (penetration: {m_rad - dists[vi]:.5f})")
            
        # Scale turret and place at mount
        turret_placed = turret_verts * m_rad + m_pos
        
        # Filter ship tris near mount
        tri_pts = ship_verts[ship_tris]
        tri_mins = np.min(tri_pts, axis=1)
        tri_maxs = np.max(tri_pts, axis=1)
        t_min = np.min(turret_placed, axis=0) - 0.01
        t_max = np.max(turret_placed, axis=0) + 0.01
        
        cand = np.where(np.all(tri_maxs >= t_min, axis=1) & np.all(tri_mins <= t_max, axis=1))[0]
        print(f"  Candidate ship triangles near turret: {len(cand)}")
        
        t_pts = turret_placed[turret_tris]
        
        intersections = []
        for s_idx in cand:
            sv0, sv1, sv2 = tri_pts[s_idx]
            s_min = np.min([sv0, sv1, sv2], axis=0)
            s_max = np.max([sv0, sv1, sv2], axis=0)
            for t_idx in range(len(turret_tris)):
                tv0, tv1, tv2 = t_pts[t_idx]
                if np.any(s_min > np.max([tv0, tv1, tv2], axis=0)) or np.any(s_max < np.min([tv0, tv1, tv2], axis=0)):
                    continue
                if tri_tri_intersect(sv0, sv1, sv2, tv0, tv1, tv2):
                    intersections.append((s_idx, t_idx))
        print(f"  Total ship-turret intersecting triangle pairs: {len(intersections)}")
        for s_idx, t_idx in intersections[:5]:
            print(f"    Ship Tri {s_idx} intersects Turret Tri {t_idx}")
