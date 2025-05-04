#include "shoot_3d.hpp"
#include <unordered_map>
#include <vector>
#include <tuple>
#include <cmath>

// Main system function - fixed to handle all collisions
void ecs_system::entity_collision(entt::registry& registry) {
    auto damageView = registry.view<Position, Body, Damage>();
    auto hpView = registry.view<Position, Body, HP>();
    
    // Check each damage-dealing entity against all HP entities
    for (auto attacker : damageView) {
        const auto& posA = damageView.get<Position>(attacker).value;
        const auto& bodyA = damageView.get<Body>(attacker);
        const auto& dmg = damageView.get<Damage>(attacker);
        
        for (auto target : hpView) {
            // Skip self-collision
            if (attacker == target) continue;
            
            const auto& posB = hpView.get<Position>(target).value;
            const auto& bodyB = hpView.get<Body>(target);
            
            // Calculate collision using direct sphere-sphere test
            Vector3 diff = Vector3Subtract(posA, posB);
            float distSq = Vector3LengthSqr(diff);
            float rSum = bodyA.radius + bodyB.radius;
            
            if (distSq <= rSum * rSum) {
                auto& hp = registry.get<HP>(target);
                hp.value -= dmg.value;
            }
        }
    }
}
