#include "classes/ui/float_stepper_widget.hpp"

namespace ui {

FloatStepperWidget::FloatStepperWidget(
    const std::string &label,
    float &valueRef,
    float minValue,
    float maxValue,
    float step,
    Rectangle bounds,
    Color color
)
    : label(label), value(&valueRef), step(step), minValue(minValue), maxValue(maxValue), bounds(bounds), color(color),
      minusButton("-", bounds, color, 20), plusButton("+", bounds, color, 20) {
    syncChildBounds();
}

void FloatStepperWidget::setBounds(Rectangle newBounds) {
    bounds = newBounds;
    syncChildBounds();
}

void FloatStepperWidget::syncChildBounds() {
    const float right = bounds.x + bounds.width;
    const float buttonWidth = bounds.width / 8.0f;
    const float valueSpaceWidth = bounds.width / 4.0f;
    const float padding = 5.0f;

    const Rectangle minusBounds = {
        right - buttonWidth - padding - valueSpaceWidth - buttonWidth,
        bounds.y + padding,
        buttonWidth,
        bounds.height - padding * 2.0f
    };
    const Rectangle plusBounds = {
        right - buttonWidth - padding,
        bounds.y + padding,
        buttonWidth,
        bounds.height - padding * 2.0f
    };

    minusButton.setBounds(minusBounds);
    minusButton.setColor(color);
    plusButton.setBounds(plusBounds);
    plusButton.setColor(color);
}

bool FloatStepperWidget::update() {
    if (!value) {
        return false;
    }

    bool changed = false;

    if (minusButton.update()) {
        *value = Clamp(*value - step, minValue, maxValue);
        changed = true;
    }

    if (plusButton.update()) {
        *value = Clamp(*value + step, minValue, maxValue);
        changed = true;
    }

    return changed;
}

void FloatStepperWidget::draw() {
    if (!value) {
        return;
    }

    DrawRectangleLinesEx(bounds, 2, ColorAlpha(color, 0.5f));
    DrawText(label.c_str(), bounds.x + 10, bounds.y + bounds.height / 2 - 10, 20, color);

    minusButton.draw();
    plusButton.draw();

    char valueText[32];
    snprintf(valueText, sizeof(valueText), "%.1f", *value);

    const float right = bounds.x + bounds.width;
    const float buttonWidth = bounds.width / 8.0f;
    const float valueSpaceWidth = bounds.width / 4.0f;
    const float padding = 5.0f;
    const int textWidth = MeasureText(valueText, 20);

    DrawText(
        valueText,
        right - buttonWidth - padding - valueSpaceWidth / 2.0f - textWidth / 2.0f,
        bounds.y + bounds.height / 2.0f - 10.0f,
        20,
        WHITE
    );
}

} // namespace ui
