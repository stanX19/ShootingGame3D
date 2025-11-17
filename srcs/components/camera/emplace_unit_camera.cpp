#include "components/unit_camera.hpp"

void camera::emplaceUnitCameraBasic(entt::registry &registry, entt::entity entity) {
	registry.emplace<UnitCamera>(entity);
}
