import json

for s in ['interceptor_quad_5ed7ec0c20dea0ac', 'heavy_quad_db2c2e68cfb939c0']:
    p = f'../scratch/model-qc/spaceships/{s}/generation_report.json'
    with open(p) as f:
        rep = json.load(f)
    print(s)
    print('  maximumRadius:', rep.get('bounds', {}).get('maximumRadius'))
    print('  engines count:', len(rep.get('resolvedEngines', [])))
    for e in rep.get('resolvedEngines', []):
        print('    engine:', e)
    print('  mounts count:', len(rep.get('mounts', [])))
    for m in rep.get('mounts', []):
        print('    mount:', m['id'], 'pos:', m['attachmentPoint'])
