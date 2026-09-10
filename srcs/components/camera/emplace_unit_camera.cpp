#include "components/camera.hpp"

void camera::emplaceUnitCameraBasic(entt::registry &registry, entt::entity entity) {
	registry.emplace<UnitCamera>(entity);
}
