#include "classes/ui/stat_bar.hpp"

#include <algorithm>


namespace
{
	Color valueColor(float ratio)
	{
		if (ratio <= 0.5f)
			return ColorLerp(SKYBLUE, GREEN, ratio / 0.5f);
		return ColorLerp(GREEN, ORANGE, (ratio - 0.5f) / 0.5f);
	}
} // namespace

namespace ui
{

	float StatBar::getClampedValue() const {
		if (maximum <= 0.0f)
			return 0.0f;
		return Clamp(value, 0.0f, maximum);
	}

	float StatBar::getNormalizedValue() const {
		return getClampedValue() / maximum;
	}

	void StatBar::draw(Rectangle bounds) const
	{
		if (bounds.width <= 0.0f || bounds.height <= 0.0f)
			return;

		constexpr float labelWidth = 90.0f;
		constexpr float valueWidth = 40.0f;
		constexpr float leftPadding = 10.0f;
		const float ratio = getNormalizedValue();
		const float barX = bounds.x + leftPadding + labelWidth + valueWidth;
		const float barWidth = std::max(
			1.0f,
			bounds.width - leftPadding - labelWidth - valueWidth);
		const float barHeight = std::min(24.0f, bounds.height);
		const float barY = bounds.y + (bounds.height - barHeight) / 2.0f;
		const int textY = static_cast<int>(
			bounds.y + (bounds.height - 16.0f) / 2.0f);
		DrawText(
			name.c_str(),
			static_cast<int>(bounds.x + leftPadding),
			textY,
			16,
			WHITE);
		DrawText(
			TextFormat("%.0f", getClampedValue()),
			static_cast<int>(bounds.x + leftPadding + labelWidth),
			textY,
			16,
			WHITE);
		// background
		DrawRectangleRec(
			Rectangle{barX, barY, barWidth, barHeight},
			ColorAlpha(BLACK, 0.5f)
		);
		// bar fill
		DrawRectangleRec(
			Rectangle{barX, barY, barWidth * ratio, barHeight},
			ColorAlpha(valueColor(ratio), 0.90f)
		);
		// outline
		DrawRectangleLinesEx(
			Rectangle{barX, barY, barWidth, barHeight},
			2.0f,
			ColorAlpha(WHITE, 0.75f)
		);
	}

} // namespace ui
