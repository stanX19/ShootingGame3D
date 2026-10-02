import glob
import numpy as np

def load_obj(path):
    verts = []
    with open(path) as f:
        for line in f:
            if line.startswith('v '):
                verts.append([float(x) for x in line.split()[1:4]])
    return np.array(verts)

for f in sorted(glob.glob("assets/Models/spaceships/*/*.obj")):
    verts = load_obj(f)
    print(f"{f}: {len(verts)} verts, X=[{verts[:,0].min():.2f}, {verts[:,0].max():.2f}], Y=[{verts[:,1].min():.2f}, {verts[:,1].max():.2f}], Z=[{verts[:,2].min():.2f}, {verts[:,2].max():.2f}]")
