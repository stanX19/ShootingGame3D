with open('scripts/gen_model/spaceship_generator.cpp') as f:
    for i, line in enumerate(f, 1):
        if 'addMounts' in line or 'SurfaceAttachment' in line:
            print(f"{i}: {line.strip()}")
