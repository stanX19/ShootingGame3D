with open('scripts/gen_model/spaceship_generator.cpp') as f:
    for i, line in enumerate(f, 1):
        if 'turret' in line.lower():
            print(f'{i}: {line.strip()}')
