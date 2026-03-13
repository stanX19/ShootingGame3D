#include "draw_utils.hpp"


bool draw_utils::isInFrontOfCamera(const Vector3 &entityPos, const Camera3D &camera)
{
	Vector3 cameraToEntity = entityPos - camera.position;
	Vector3 forward = camera.target - camera.position;
	return Vector3DotProduct(cameraToEntity, forward) > 0;
}

bool draw_utils::draw_text_button(const char *text, Rectangle bounds, Color color, int fontSize)
{
	bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
	bool down = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
	bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
	
	Color drawCol = ColorAlpha(color, 0.4f + (hovered + (hovered && down)) * 0.2f);

	DrawRectangleRec(bounds, drawCol);
	DrawRectangleLinesEx(bounds, 2, color);
	
	int textWidth = MeasureText(text, fontSize);
	DrawText(text, bounds.x + bounds.width / 2 - textWidth / 2, bounds.y + bounds.height / 2 - fontSize / 2, fontSize, color);
	
	return hovered && clicked;
}