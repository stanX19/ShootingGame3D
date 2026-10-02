# Simulate gap = 0.1f on turret_generator.cpp parameters

bRad = 1.0
gap = 0.10

# Pivot ball details
sphereRadius = bRad
cheekW = sphereRadius * 0.16
cheekH = sphereRadius * 0.42
cheekL = sphereRadius * 0.72
cheekX = sphereRadius + gap + cheekW * 0.5
cheek_inner_X = cheekX - cheekW * 0.5

hubR = sphereRadius * 0.32
hubInnerX = cheekX + cheekW * 0.5 + gap
hubOuterX = hubInnerX + sphereRadius * 0.09

neonInnerX = hubOuterX + gap
neonOuterX = neonInnerX + sphereRadius * 0.02

breechW = sphereRadius * 1.12
breechH = sphereRadius * 0.86
breechL = sphereRadius * 0.75 # Adjusted from 0.90 to keep rear within -2.20
breechZ = -sphereRadius - gap - breechL * 0.5
breech_front_Z = breechZ + breechL * 0.5

exhaustR = sphereRadius * 0.17
exhaustStartZ = breechCenter_z = breechZ - breechL * 0.5 - gap
exhaustEndZ = exhaustStartZ - sphereRadius * 0.25

print(f"Cheek inner X: {cheek_inner_X:.3f} (clearance: {cheek_inner_X - sphereRadius:.3f})")
print(f"Hub outer X: {hubOuterX:.3f}")
print(f"Neon outer X: {neonOuterX:.3f}")
print(f"Breech front Z: {breech_front_Z:.3f} (clearance: {-sphereRadius - breech_front_Z:.3f})")
print(f"Exhaust end Z: {exhaustEndZ:.3f}")

# Basic shooter
mantletW = bRad * 1.24
mantletH = bRad * 0.98
mantletL = bRad * 0.62
mantletZ = bRad + gap + mantletL * 0.5
mantlet_back_Z = mantletZ - mantletL * 0.5
print(f"Mantlet back Z: {mantlet_back_Z:.3f} (clearance: {mantlet_back_Z - bRad:.3f})")
