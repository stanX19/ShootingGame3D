#include "raylib.h"
#include "mesh_manager.hpp"
#include <iostream>

int main() {
    // Init Raylib window
    InitWindow(800, 600, "MeshManager Test");
    SetTargetFPS(60);
    Camera camera;
    camera.position = Vector3{ 0.0f, 1.0f, 4.0f };
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Create MeshManager instance
    MeshManager meshManager;

    // Create models (some duplicates to test caching)
    t_mesh_id box1 = meshManager.createBox(1.0f, 1.0f, 1.0f);
    t_mesh_id box2 = meshManager.createBox(1.0f, 1.0f, 1.0f);  // Should return same ID

    t_mesh_id sphere1 = meshManager.createSphere(1.0f, 16, 16);
    t_mesh_id sphere2 = meshManager.createSphere(1.0f, 17, 16);
    t_mesh_id plane = meshManager.createPlane(4.0f, 4.0f, 4, 4);

    std::cout << "Start of load" << "\n";
	t_mesh_id ship = meshManager.loadModel("assets/Models/spacechip1/model/Intergalactic_Spaceship-(Wavefront).obj");
    std::cout << "End of load" << "\n";
    // Print results
    std::cout << "Box1 ID: " << box1 << "\n";
    std::cout << "Box2 ID (should match Box1): " << box2 << "\n";
    std::cout << "Sphere1 ID: " << sphere1 << "\n";
    std::cout << "Sphere2 ID: " << sphere2 << "\n";
    std::cout << "Plane ID: " << plane << "\n";

    // Main render loop
    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode3D(camera);

        DrawModel(meshManager.getModel(box1), { -2.0f, 0.0f, 0.0f }, 1.0f, RED);
        DrawModel(meshManager.getModel(ship), { 0.0f, 0.0f, 0.0f }, 1.0f, GREEN);
        DrawModel(meshManager.getModel(plane), { 0.0f, -1.0f, 0.0f }, 1.0f, LIGHTGRAY);

        EndMode3D();
        DrawText("MeshManager Test - Press ESC to exit", 10, 10, 20, DARKGRAY);
        EndDrawing();
    }

    // Cleanup
    CloseWindow();  // Also calls meshManager destructor and unloads models
    return 0;
}
