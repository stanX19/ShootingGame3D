#include "utils.hpp"
#include <iostream>

Vector2 getMouseRatioRelCenter()
{
    Vector2 mousePos = GetMousePosition();
	// std::cout << mousePos.x << ' ' << mousePos.y << std::endl;
	Vector2 screenCenter = Vector2{GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
	// std::cout << centerX << ' ' << centerY << std::endl;
	Vector2 relPos = mousePos - screenCenter;
    return relPos / screenCenter;
}

Vector2 getMouseDirectionNormalized(float clampRatio)
{
    Vector2 mousePos = GetMousePosition();
	Vector2 screenCenter = Vector2{GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
	Vector2 relPos = mousePos - screenCenter;
	float clampDist = std::min(screenCenter.x, screenCenter.y) * clampRatio;
    return Vector2Normalize(relPos) * std::min(1.0f, Vector2Length(relPos) / clampDist);
}