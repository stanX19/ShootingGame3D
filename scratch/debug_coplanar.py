import json

# Let's inspect interceptor_quad obj
with open("assets/Models/spaceships/interceptor_quad/spaceship_interceptor_quad.obj") as f:
    lines = f.readlines()

positions = []
faces = []
for line in lines:
    if line.startswith("v "):
        parts = [float(p) for p in line.strip().split()[1:]]
        positions.append(parts)
    elif line.startswith("f "):
        # f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
        parts = line.strip().split()[1:]
        indices = [int(p.split('/')[0]) - 1 for p in parts]
        faces.append(indices)

print(f"Total positions: {len(positions)}, total faces: {len(faces)}")
if 4862 < len(faces) and 4758 < len(faces):
    f_left = faces[4862]
    f_right = faces[4758]
    print(f"Face 4862 vertices:")
    for idx in f_left:
        print(f"  v[{idx}] = {positions[idx]}")
    print(f"Face 4758 vertices:")
    for idx in f_right:
        print(f"  v[{idx}] = {positions[idx]}")
else:
    print("Indices out of range of obj faces")
