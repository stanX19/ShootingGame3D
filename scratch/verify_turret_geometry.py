import os
import sys
import numpy as np

def verify_turret(obj_path, expected_radius=1.0):
    if not os.path.exists(obj_path):
        print(f"ERROR: File does not exist: {obj_path}")
        return False

    verts = []
    with open(obj_path, 'r') as f:
        for line in f:
            if line.startswith('v '):
                parts = line.strip().split()
                verts.append([float(parts[1]), float(parts[2]), float(parts[3])])

    verts = np.array(verts)
    print(f"\n=======================================================")
    print(f" GEOMETRIC VERIFICATION EVIDENCE: {os.path.basename(obj_path)}")
    print(f" Path: {obj_path}")
    print(f" Total Vertices: {len(verts)}")
    print(f"=======================================================")

    # First 165 vertices are the pivot sphere: 11 rings * 15 sectors
    sphere_verts = verts[:165]
    dists = np.linalg.norm(sphere_verts, axis=1)
    sphere_center = (sphere_verts.min(axis=0) + sphere_verts.max(axis=0)) * 0.5
    min_r = dists.min()
    max_r = dists.max()
    mean_r = dists.mean()

    print(f"[RULE 1] Origin / Center Verification:")
    print(f"  Authored Pivot Center:  (0.000000, 0.000000, 0.000000)")
    print(f"  Measured Sphere Center: ({sphere_center[0]:.6f}, {sphere_center[1]:.6f}, {sphere_center[2]:.6f})")
    center_ok = np.allclose(sphere_center, [0, 0, 0], atol=1e-4)
    print(f"  Center Check:           {'[PASS] EXACT MATCH (0, 0, 0)' if center_ok else '[FAIL]'}")

    print(f"\n[RULE 2] Base Sphere Radius Verification:")
    print(f"  Expected Base Radius:   {expected_radius:.6f}")
    print(f"  Measured Min Radius:    {min_r:.6f}")
    print(f"  Measured Max Radius:    {max_r:.6f}")
    print(f"  Measured Mean Radius:   {mean_r:.6f}")
    radius_ok = np.isclose(min_r, expected_radius, atol=1e-4) and np.isclose(max_r, expected_radius, atol=1e-4)
    print(f"  Radius Check:           {'[PASS] EXACT MATCH 1.0f' if radius_ok else '[FAIL]'}")

    print(f"\n[OVERALL MODEL BOUNDS]")
    min_b = verts.min(axis=0)
    max_b = verts.max(axis=0)
    extents = max_b - min_b
    print(f"  X Range: [{min_b[0]:.4f}, {max_b[0]:.4f}] (Width:  {extents[0]:.4f})")
    print(f"  Y Range: [{min_b[1]:.4f}, {max_b[1]:.4f}] (Height: {extents[1]:.4f})")
    print(f"  Z Range: [{min_b[2]:.4f}, {max_b[2]:.4f}] (Length: {extents[2]:.4f})")
    print(f"=======================================================\n")
    return center_ok and radius_ok

if __name__ == '__main__':
    base_dir = sys.argv[1] if len(sys.argv) > 1 else 'scratch/model-qc/turrets'
    all_ok = True
    for tid in ['basic_shooter', 'heavy_mg_rifle', 'lazer_deletor', 'lazer_shotgun']:
        p = os.path.join(base_dir, tid, f"turret_{tid}.obj")
        ok = verify_turret(p, 1.0)
        all_ok = all_ok and ok
    sys.exit(0 if all_ok else 1)
