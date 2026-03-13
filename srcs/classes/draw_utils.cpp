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

bool draw_utils::draw_value_button(const char *label, float &value, float step, float min, float max, Rectangle bounds, Color color)
{
	DrawRectangleLinesEx(bounds, 2, ColorAlpha(color, 0.5f));
	
	// Label
	DrawText(label, bounds.x + 10, bounds.y + bounds.height / 2 - 10, 20, color);
	
	float right = bounds.x + bounds.width;
	float buttonWidth = bounds.width / 8;
	float valueSpaceWidth = bounds.width / 4;
	float padding = 5.0f;
	Rectangle btnMinus = { right - buttonWidth - padding - valueSpaceWidth - buttonWidth, bounds.y + padding, buttonWidth, bounds.height - padding * 2 };
	Rectangle btnPlus = { right - buttonWidth - padding, bounds.y + padding, buttonWidth, bounds.height - padding * 2 };
	
	bool changed = false;
	
	if (draw_text_button("-", btnMinus, color, 20)) {
		value -= step;
		if (value < min)
			value = min;
		changed = true;
	}
	if (draw_text_button("+", btnPlus, color, 20)) {
		value += step;
		if (value > max)
			value = max;
		changed = true;
	}
	
	char valBuf[32];
	sprintf(valBuf, "%.1f", value);
	int valWidth = MeasureText(valBuf, 20);
	DrawText(valBuf, right - buttonWidth - padding - valueSpaceWidth / 2 - valWidth / 2, bounds.y + bounds.height / 2 - 10, 20, WHITE);
	
	
	return changed;
}