import glob
import os

def parse_bounds_and_faces(obj_path):
    xs, ys, zs = [], [], []
    faces = 0
    with open(obj_path, 'r') as f:
        for line in f:
            if line.startswith('v '):
                parts = line.strip().split()
                xs.append(float(parts[1]))
                ys.append(float(parts[2]))
                zs.append(float(parts[3]))
            elif line.startswith('f '):
                faces += 1
    return {
        'faces': faces,
        'min': (min(xs), min(ys), min(zs)),
        'max': (max(xs), max(ys), max(zs))
    }

collision_files = glob.glob('../scratch/model-qc/spaceships/*/*.collision.obj')
collision_files.sort()

all_passed = True
print(f"{'Ship':<25} | {'Tris':<6} | {'Collision Enclosed Inside Visual?':<35}")
print("-" * 75)

for col_path in collision_files:
    vis_path = col_path.replace('.collision.obj', '.obj')
    ship_name = os.path.basename(col_path).replace('.collision.obj', '')
    if not os.path.exists(vis_path):
        continue
    
    col_info = parse_bounds_and_faces(col_path)
    vis_info = parse_bounds_and_faces(vis_path)
    
    col_min, col_max = col_info['min'], col_info['max']
    vis_min, vis_max = vis_info['min'], vis_info['max']
    
    # Check if collision bounds are inside visual bounds (with small epsilon for floating point)
    eps = 0.05
    inside_x = (col_min[0] >= vis_min[0] - eps) and (col_max[0] <= vis_max[0] + eps)
    inside_y = (col_min[1] >= vis_min[1] - eps) and (col_max[1] <= vis_max[1] + eps)
    inside_z = (col_min[2] >= vis_min[2] - eps) and (col_max[2] <= vis_max[2] + eps)
    
    is_inside = inside_x and inside_y and inside_z
    if not is_inside:
        all_passed = False
    
    status = "YES (Strictly Inside)" if is_inside else f"NO (col: {col_min}..{col_max}, vis: {vis_min}..{vis_max})"
    print(f"{ship_name:<25} | {col_info['faces']:<6} | {status}")

print("-" * 75)
print(f"All ships strictly enclosed: {all_passed}")

if all_passed:
    import shutil
    print("\nPromoting collision models to assets/models/spaceships/...")
    for col_path in collision_files:
        ship_name = os.path.basename(col_path).replace('.collision.obj', '').replace('spaceship_', '')
        dest_dir = f"assets/models/spaceships/{ship_name}"
        if os.path.exists(dest_dir):
            dest_file = os.path.join(dest_dir, os.path.basename(col_path))
            shutil.copyfile(col_path, dest_file)
            print(f"Promoted: {dest_file} ({os.path.getsize(dest_file)} bytes)")
