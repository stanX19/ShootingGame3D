#ifndef MESH_MANAGER_HPP
#define MESH_MANAGER_HPP

#include "includes.hpp"
#include "components.hpp"
#include "constants.hpp"
#include "utils.hpp"

using t_mesh_id = size_t;

class MeshManager {
public:
    MeshManager();
    ~MeshManager();

    t_mesh_id loadModel(const std::string& filePath);

    t_mesh_id createBox(float width, float height, float length);
    t_mesh_id createSphere(float radius, int rings, int slices);
    t_mesh_id createPlane(float width, float length, int resX, int resZ);

	
    Model& getModel(t_mesh_id id);
    const Model& getModel(t_mesh_id id) const;

    void unloadAll();

    bool isValid(t_mesh_id id) const;
private:
    std::vector<Model> models;
    std::unordered_map<std::string, t_mesh_id> loadedFromFile; // filepath -> id
	std::unordered_map<std::string, t_mesh_id> proceduralCache;
	
	template<typename... Args>
	bool paramExists(const std::string& keyBase, t_mesh_id& outId, Args&&... args);
};

#endif