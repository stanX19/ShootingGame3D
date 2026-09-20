import glob

for f in sorted(glob.glob("assets/Models/**/*.collision.obj", recursive=True)):
    xs, ys, zs = [], [], []
    with open(f) as fp:
        for line in fp:
            if line.startswith("v "):
                parts = [float(p) for p in line.strip().split()[1:4]]
                xs.append(parts[0])
                ys.append(parts[1])
                zs.append(parts[2])
    if xs:
        print(f"{f}: count={len(xs)} x=[{min(xs):.2f}, {max(xs):.2f}] y=[{min(ys):.2f}, {max(ys):.2f}] z=[{min(zs):.2f}, {max(zs):.2f}]")
