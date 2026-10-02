import json
import glob

for f in sorted(glob.glob("assets/Models/spaceships/*/generation_report.json")):
    with open(f) as fp:
        data = json.load(fp)
        mounts = data.get("resolvedMounts", [])
        print(f"{f}: {len(mounts)} mounts")
        for m in mounts:
            print(f"  mount {m['id']}: pos={m['position']}, rad={m['turretRadius']}")
