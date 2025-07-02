#include "includes.hpp"
#include <iostream>

int main()
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Mouse Debug");

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();
		ClearBackground(RAYWHITE);
		Vector2 mousePos = GetMousePosition(); // Use GetMousePosition()
		DrawText(TextFormat("Mouse X: %.0f", mousePos.x), 10, 10, 20, BLACK);
		DrawText(TextFormat("Mouse Y: %.0f", mousePos.y), 10, 40, 20, BLACK);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}