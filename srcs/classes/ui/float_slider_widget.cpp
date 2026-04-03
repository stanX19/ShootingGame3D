#include "classes/ui/float_slider_widget.hpp"
#include <cmath>

namespace ui {

FloatSliderWidget::FloatSliderWidget(
    const std::string &label,
    float &valueRef,
    float minValue,
    float maxValue,
    float step,
    Rectangle bounds,
    Color color
)
    : label(label), value(&valueRef), minValue(minValue), maxValue(maxValue), step(step), bounds(bounds), color(color) {
}

void FloatSliderWidget::setBounds(Rectangle newBounds) {
    bounds = newBounds;
}

Rectangle FloatSliderWidget::getTrackBounds() const {
    const float sliderLeft = bounds.x + bounds.width / 2.0f + 5.0f;
    const float sliderRight = bounds.x + bounds.width - 10.0f;
    const float sliderWidth = std::max(1.0f, sliderRight - sliderLeft);

    return Rectangle{
        sliderLeft,
        bounds.y + bounds.height / 2.0f - 3.0f,
        sliderWidth,
        6.0f
    };
}

float FloatSliderWidget::getKnobX(const Rectangle &trackBounds) const {
    if (!value || maxValue <= minValue) {
        return trackBounds.x;
    }

    const float clampedValue = Clamp(*value, minValue, maxValue);
    const float normalized = (clampedValue - minValue) / (maxValue - minValue);
    return trackBounds.x + trackBounds.width * normalized;
}

float FloatSliderWidget::getSnappedValueFromMouseX(const Rectangle &trackBounds, float mouseX) const {
    if (maxValue <= minValue) {
        return minValue;
    }

    const float rawNormalized = (mouseX - trackBounds.x) / trackBounds.width;
    const float clampedNormalized = Clamp(rawNormalized, 0.0f, 1.0f);
    const float rawValue = minValue + (maxValue - minValue) * clampedNormalized;

    return Clamp(std::round(rawValue / step) * step, minValue, maxValue);
}

bool FloatSliderWidget::update() {
    if (!value) {
        return false;
    }

    *value = Clamp(*value, minValue, maxValue);
    const Rectangle trackBounds = getTrackBounds();
    const Rectangle interactionBounds = {
        trackBounds.x,
        bounds.y,
        trackBounds.width,
        bounds.height
    };

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)
        && CheckCollisionPointRec(GetMousePosition(), interactionBounds)) {
        isDragging = true;
    }

    bool changed = false;

    if (isDragging && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        const float snappedValue = getSnappedValueFromMouseX(trackBounds, GetMousePosition().x);
        if (std::abs(snappedValue - *value) > 0.0001f) {
            *value = snappedValue;
            changed = true;
        }
    }

    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        isDragging = false;
    }

    return changed;
}

void FloatSliderWidget::draw() {
    if (!value) {
        return;
    }

    DrawRectangleLinesEx(bounds, 2.0f, ColorAlpha(color, 0.5f));
    DrawText(label.c_str(), bounds.x + 10.0f, bounds.y + bounds.height / 2.0f - 10.0f, 20, color);

    const Rectangle trackBounds = getTrackBounds();
    DrawRectangleRec(trackBounds, ColorAlpha(color, 0.35f));

    const float knobX = getKnobX(trackBounds);
    const Rectangle knob = {
        knobX - 6.0f,
        bounds.y + 6.0f,
        12.0f,
        bounds.height - 12.0f
    };
    DrawRectangleRec(knob, ColorAlpha(color, isDragging ? 1.0f : 0.85f));
}

} // namespace ui
