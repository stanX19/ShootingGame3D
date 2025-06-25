#include "raylib.h"
#include "model_manager.hpp"
#include <iostream>

int main() {
	SetTraceLogLevel(LOG_ALL);
    // Init Raylib window
    InitWindow(800, 600, "ModelManager Test");
    SetTargetFPS(60);
    Camera camera;
    camera.position = Vector3{ 0.0f, 3.0f, 4.0f } * 10;
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Create ModelManager instance
    ModelManager meshManager;

    // Create models (some duplicates to test caching)
    t_model_id box1 = meshManager.createBox(1.0f, 1.0f, 1.0f);
    t_model_id box2 = meshManager.createBox(1.0f, 1.0f, 1.0f);  // Should return same ID

    t_model_id sphere1 = meshManager.createSphere(16, 16, 1.0f);
    t_model_id sphere2 = meshManager.createSphere(17, 16, 1.0f);
    t_model_id plane = meshManager.createPlane(4.0f, 4.0f, 4, 4);

    std::cout << "Start of load" << "\n";
	// 	t_model_id ship = meshManager.loadModel("assets/Models/spaceship2/Intergalactic_Spaceships_Version_2.gltf");
	// t_model_id ship = meshManager.loadModel("assets/Models/spaceship_custom_100/spaceship.obj");
	t_model_id ship = meshManager.loadModel("assets/Models/spacechip1/model/Intergalactic_Spaceship-(Wavefront).obj");
    std::cout << "End of load" << "\n";
	t_model_id ship_id_2 = meshManager.loadModel("assets/Models/spacechip1/model/Intergalactic_Spaceship-(Wavefront).obj");

    // Print results
    std::cout << "Box1 ID: " << box1 << "\n";
    std::cout << "Box2 ID (should match Box1): " << box2 << "\n";
    std::cout << "Sphere1 ID: " << sphere1 << "\n";
    std::cout << "Sphere2 ID: " << sphere2 << "\n";
    std::cout << "Ship1 ID: " << ship << "\n";
    std::cout << "Ship2 ID: " << ship_id_2 << "\n";
    std::cout << "Plane ID: " << plane << "\n";

    // Main render loop
    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode3D(camera);

        DrawModel(meshManager.getModel(sphere1), { -10.0f, 0.0f, 0.0f }, 1.0f, Color{ 230, 41, 55, 105 });
        DrawModel(meshManager.getModel(ship), { 0.0f, 0.0f, 0.0f }, 0.4f, WHITE);
        DrawModel(meshManager.getModel(plane), { 0.0f, -1.0f, 0.0f }, 1.0f, LIGHTGRAY);

        EndMode3D();
        DrawText("ModelManager Test - Press ESC to exit", 10, 10, 20, DARKGRAY);
        EndDrawing();
    }

    // Cleanup
	meshManager.unloadAll();
    CloseWindow();  // Also calls meshManager destructor and unloads models
    return 0;
}
