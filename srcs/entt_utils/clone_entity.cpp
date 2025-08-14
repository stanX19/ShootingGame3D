#include "entt_utils.hpp"
#include <iostream>

entt::entity entt_utils::cloneEntity(entt::registry &src, entt::entity srcEntity, entt::registry &dst) {
	entt::entity dstEntity = dst.create();
	
    for (auto [id, storage] : src.storage()) {
		if (!storage.contains(srcEntity))
			continue;
		auto* dst_storage = dst.storage(id);
		if (!dst_storage) {
			throw std::runtime_error("cloneEntity failed due to null dst storage");
		}
		dst_storage->push(dstEntity, storage.value(srcEntity));
    }

	return dstEntity;
}
