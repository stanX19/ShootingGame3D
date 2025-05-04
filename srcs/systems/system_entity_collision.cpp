#include "shoot_3d.hpp"
#include <unordered_map>
#include <vector>
#include <tuple>
#include <cmath>

struct OctreeNode {
    Vector3 center;
    float halfSize;
    std::vector<entt::entity> entities;
    std::unique_ptr<OctreeNode> children[8];
    static constexpr int MAX_ENTITIES = 8;
    static constexpr int MAX_DEPTH = 5;

    OctreeNode(Vector3 center, float halfSize)
        : center(center), halfSize(halfSize) {}

    bool contains(const Vector3& point) const {
        return (fabs(point.x - center.x) <= halfSize &&
                fabs(point.y - center.y) <= halfSize &&
                fabs(point.z - center.z) <= halfSize);
    }

    int getOctant(const Vector3& point) const {
        return (point.x > center.x ? 1 : 0) |
               (point.y > center.y ? 2 : 0) |
               (point.z > center.z ? 4 : 0);
    }

    void subdivide() {
        float q = halfSize / 2.0f;
        for (int i = 0; i < 8; ++i) {
            Vector3 offset = {
                (i & 1 ? q : -q),
                (i & 2 ? q : -q),
                (i & 4 ? q : -q)
            };
            children[i] = std::make_unique<OctreeNode>(
                Vector3Add(center, offset), q);
        }
    }

    void insert(entt::entity e, const Vector3& pos, int depth = 0) {
        if (!contains(pos)) return;

        if (entities.size() < MAX_ENTITIES || depth >= MAX_DEPTH) {
            entities.push_back(e);
        } else {
            if (!children[0]) subdivide();
            int oct = getOctant(pos);
            children[oct]->insert(e, pos, depth + 1);
        }
    }

    void query(const Vector3& pos, float radius, std::vector<entt::entity>& out) const {
        if (!intersects(pos, radius)) return;

        for (entt::entity e : entities) {
            out.push_back(e);
        }

        if (children[0]) {
            for (auto& child : children) {
                child->query(pos, radius, out);
            }
        }
    }

    bool intersects(const Vector3& pos, float radius) const {
        float d = 0;
        for (int i = 0; i < 3; ++i) {
            float v = (&pos.x)[i];
            float c = (&center.x)[i];
            float min = c - halfSize;
            float max = c + halfSize;
            if (v < min) d += (min - v) * (min - v);
            else if (v > max) d += (v - max) * (v - max);
        }
        return d <= radius * radius;
    }
};

// Main system function
void ecs_system::entity_collision(entt::registry& registry) {
    auto damageView = registry.view<Position, Body, Damage>();
    auto hpView = registry.view<Position, Body, HP>();

    // Build octree
    OctreeNode root({0, 0, 0}, 100.0f); // Adjust size to your arena

    for (auto entity : hpView) {
        const auto& pos = hpView.get<Position>(entity).value;
        root.insert(entity, pos);
    }

    for (auto attacker : damageView) {
        const auto& posA = damageView.get<Position>(attacker).value;
        const auto& bodyA = damageView.get<Body>(attacker);
        const auto& dmg = damageView.get<Damage>(attacker);

        std::vector<entt::entity> nearby;
        root.query(posA, bodyA.radius + 5.0f, nearby);

        for (auto target : nearby) {
            if (attacker == target) continue;

            const auto& posB = registry.get<Position>(target).value;
            const auto& bodyB = registry.get<Body>(target);

            Vector3 diff = Vector3Subtract(posA, posB);
            float distSq = Vector3LengthSqr(diff);
            float rSum = bodyA.radius + bodyB.radius;

            if (distSq <= rSum * rSum) {
                auto& hp = registry.get<HP>(target);
                hp.value -= dmg.value;
                break;
            }
        }
    }
}