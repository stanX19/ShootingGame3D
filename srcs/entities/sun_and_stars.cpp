#include "entities.hpp"
#include "utils.hpp"
#include "constants.hpp"


void spawnSunAndStars(GameContext &context, int numStars) {
	entt::entity sun2 = context.registry.create();
	Position pos = {randomUnitVector3() * ARENA_SIZE * 3};
	// t_model_id sunModel = context.modelManager.loadModel("assets/Models/sun/sun.glb");
	t_model_id sunModel = context.modelManager.createSphere(128, 128);
	float rad = GetRandomValue(ARENA_SIZE * 0.3, ARENA_SIZE * 0.5);

	context.registry.emplace<Position>(sun2, pos);
	context.registry.emplace<RenderBody>(sun2, sunModel, Color{105, 205, 255, 255}, rad);
	context.registry.emplace<tag::LightSource>(sun2);
	
	// stars
	t_model_id starsModel = context.modelManager.createSphere();

	for (int i = 0; i < numStars; i++) {
		entt::entity entity = context.registry.create();

		context.registry.emplace<Position>(entity, randomUnitVector3() * ARENA_SIZE * 5);
		context.registry.emplace<RenderBody>(entity, starsModel, WHITE, (GetRandomValue(15, 25) * ARENA_SIZE / 4000.0f));
	}

	// space
	t_model_id spaceModel = context.modelManager.loadModel("assets/Models/space/sky_pano_-_milkyway.glb");
	Model model = context.modelManager.getModel(spaceModel);
	model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = model.materials[1].maps[MATERIAL_MAP_EMISSION].texture;
	model.materials[1].maps[MATERIAL_MAP_ALBEDO].texture = model.materials[1].maps[MATERIAL_MAP_EMISSION].texture;

	entt::entity space = context.registry.create();
	context.registry.emplace<Position>(space);
	context.registry.emplace<RenderBody>(space, RenderBody{spaceModel, 1.0f});
	context.registry.emplace<tag::SkyBox>(space);
}