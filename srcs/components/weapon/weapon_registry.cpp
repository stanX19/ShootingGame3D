#include "weapon_registry.hpp"
#include "weapons.hpp"
#include "basic_utils.hpp"

const std::vector<weapon::WeaponDescriptor>& weapon::getAllBulletWeapons() {
    static const std::vector<WeaponDescriptor> registry = {
        {"Basic", "Bullet", emplaceWeaponBasic},
        {"Sniper", "Bullet", emplaceWeaponSniper},
        {"Burst Sniper", "Bullet", emplaceWeaponBurstSniper},
        {"Machine Gun", "Bullet", emplaceWeaponMachineGun},
        {"Shotgun", "Bullet", emplaceWeaponShotgun},
        {"Asteroid Launcher", "Bullet", emplaceWeaponBigBall},
    };
    return registry;
}

const std::vector<weapon::WeaponDescriptor>& weapon::getAllLazerWeapons() {
    static const std::vector<WeaponDescriptor> registry = {
        {"Lazer", "Lazer", emplaceWeaponLazerBasic},
        {"Lazer Machine Gun", "Lazer", emplaceWeaponLazerMachineGun},
        {"Deletor", "Lazer", emplaceWeaponLazerDeletor},
        {"Lazer Shotgun", "Lazer", emplaceWeaponLazerShotgun},
    };
    return registry;
}

const std::vector<weapon::WeaponDescriptor>& weapon::getAllMissileWeapons() {
    static const std::vector<WeaponDescriptor> registry = {
        {"Basic Missile", "Missile", emplaceWeaponMissileBasic},
        {"Swarm Missile", "Missile", emplaceWeaponMissileSwarm},
        {"Torpedo", "Missile", emplaceWeaponMissileTorpedo},
        {"Nuke", "Missile", emplaceWeaponMissileNuke},
        {"Sniper Missile", "Missile", emplaceWeaponMissileSniper},
        {"Flares", "Missile", emplaceWeaponMissileFlares}
    };
    return registry;
}

const std::vector<weapon::WeaponDescriptor>& weapon::getAllWeapons() {
    static const std::vector<WeaponDescriptor> registry = merge_vectors(
        getAllBulletWeapons(),
        getAllLazerWeapons(),
        getAllMissileWeapons()
    );
    return registry;
}

const std::vector<weapon::WeaponDescriptor>& weapon::getAllAttackWeapons() {
    static const std::vector<WeaponDescriptor> registry = merge_vectors(
        getAllBulletWeapons(),
        getAllLazerWeapons()
    );
    return registry;
}

void weapon::emplaceRandomAttackWeapon(GameContext &context, entt::entity entity) {
    emplaceRandomAttackWeapon(context, entity, GetRandomValue(0, 100000));
}

void weapon::emplaceRandomAttackWeapon(GameContext &context, entt::entity entity, int value) {
    const auto& list = getAllAttackWeapons();
    list[value % list.size()].emplaceFunc(context, entity);
}

void weapon::emplaceRandomMissileWeapon(GameContext &context, entt::entity entity) {
    emplaceRandomMissileWeapon(context, entity, GetRandomValue(0, 100000));
}

void weapon::emplaceRandomMissileWeapon(GameContext &context, entt::entity entity, int value) {
    const auto& list = getAllMissileWeapons();
    list[value % list.size()].emplaceFunc(context, entity);
}
