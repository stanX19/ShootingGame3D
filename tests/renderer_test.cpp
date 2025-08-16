#include "includes.hpp"
#include "game_context.hpp"
#include "renderer.hpp"
#include "components.hpp"
#include "utils.hpp" // For random functions if you want to spawn multiple models

int main()
{
	// Initialization
	//--------------------------------------------------------------------------------------
	const int screenWidth = 1280;
	const int screenHeight = 720;

	InitWindow(screenWidth, screenHeight, "Raylib 3D Model Renderer Test");

	// Define the camera to look into our 3D world
	Camera camera;
	camera.position = Vector3{ 0.0f, 3.0f, 4.0f } * 2;
	camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
	camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
	camera.fovy = 45.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	// Game Context and Renderer setup
	GameContext context;
	Renderer renderer(camera, context);

	// Create a light source (required for your shader)
	auto light = context.registry.create();
	context.registry.emplace<Position>(light, Vector3{100.0f, 100.0f, 100.0f});
	context.registry.emplace<RenderBody>(light, context.modelManager.createSphere(), WHITE, 1.0f); // Light color
	context.registry.emplace<tag::LightSource>(light);

	// Create an entity with a ModelData component
	entt::entity testModelEntity = context.registry.create();
	context.registry.emplace<Position>(testModelEntity, Vector3{0.0f, 0.0f, 0.0f});
	context.registry.emplace<RenderBody>(testModelEntity, context.modelManager.createSphere(), RED, 1.0f); // RenderBody for general rendering, not directly used by ModelData
	context.registry.emplace<tag::Shaded>(testModelEntity);		   // Tag to ensure it's drawn with the shader

	t_model_id boxModelID = context.modelManager.createCube(2.0f, 2.0f, 2.0f);

	// Create a second entity to test another model
	entt::entity secondTestModelEntity = context.registry.create();
	context.registry.emplace<Position>(secondTestModelEntity, Vector3{3.0f, 0.0f, 0.0f});
	context.registry.emplace<tag::Shaded>(secondTestModelEntity);
	context.registry.emplace<RenderBody>(secondTestModelEntity, RenderBody{
		boxModelID,
		BLUE,
		1.0,
		Vector3{0.0f, 0.0f, 0.0f},
		QuaternionFromEuler(DEG2RAD * 45.0f, DEG2RAD * 0.0f, DEG2RAD * 45.0f), // Rotate 45 degrees around X
	});

	SetTargetFPS(60); // Set our game to run at 60 frames-per-second
	//--------------------------------------------------------------------------------------

	// Game loop
	while (!WindowShouldClose()) // Detect window close button or ESC key
	{
		// Update
		//----------------------------------------------------------------------------------
		// UpdateCamera(&camera, CAMERA_ORBITAL); // Update camera position (for user interaction)

		// Rotate the box over time
		auto& modelDataBox = context.registry.get<RenderBody>(secondTestModelEntity);
		modelDataBox.rotation = QuaternionFromAxisAngle(Vector3{0.0f, 1.0f, 0.0f}, GetTime() * DEG2RAD * 1000.0f);

		// Draw
		//----------------------------------------------------------------------------------
		renderer.Render(); // Use the existing Renderer::Render method

		// Drawing specific to main for testing purposes if needed
		BeginDrawing();
		DrawText("Welcome to the ModelData Test!", 10, 50, 20, GREEN);
		DrawFPS(10, 10);
		EndDrawing();
		//----------------------------------------------------------------------------------
	}

	// De-Initialization
	//--------------------------------------------------------------------------------------
	context.modelManager.unloadAll();
	CloseWindow(); // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

	return 0;
}
