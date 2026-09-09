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
    : m_label(label), m_value(&valueRef), m_step(step), m_minValue(minValue), m_maxValue(maxValue), m_bounds(bounds), m_color(color),
      m_minusButton("-", bounds, color, 20), m_plusButton("+", bounds, color, 20) {
    syncChildBounds();
}

void FloatStepperWidget::setBounds(Rectangle newBounds) {
    m_bounds = newBounds;
    syncChildBounds();
}

void FloatStepperWidget::syncChildBounds() {
    const float right = m_bounds.x + m_bounds.width;
    const float buttonWidth = m_bounds.width / 8.0f;
    const float valueSpaceWidth = m_bounds.width / 4.0f;
    const float padding = 5.0f;

    const Rectangle minusBounds = {
        right - buttonWidth - padding - valueSpaceWidth - buttonWidth,
        m_bounds.y + padding,
        buttonWidth,
        m_bounds.height - padding * 2.0f
    };
    const Rectangle plusBounds = {
        right - buttonWidth - padding,
        m_bounds.y + padding,
        buttonWidth,
        m_bounds.height - padding * 2.0f
    };

    m_minusButton.setBounds(minusBounds);
    m_minusButton.setColor(m_color);
    m_plusButton.setBounds(plusBounds);
    m_plusButton.setColor(m_color);
}

bool FloatStepperWidget::update() {
    if (!m_value) {
        return false;
    }

    bool changed = false;

    if (m_minusButton.update()) {
        *m_value = Clamp(*m_value - m_step, m_minValue, m_maxValue);
        changed = true;
    }

    if (m_plusButton.update()) {
        *m_value = Clamp(*m_value + m_step, m_minValue, m_maxValue);
        changed = true;
    }

    return changed;
}

void FloatStepperWidget::draw() {
    if (!m_value) {
        return;
    }

    DrawRectangleLinesEx(m_bounds, 2, ColorAlpha(m_color, 0.5f));
    DrawText(m_label.c_str(), m_bounds.x + 10, m_bounds.y + m_bounds.height / 2 - 10, 20, m_color);

    m_minusButton.draw();
    m_plusButton.draw();

    char valueText[32];
    snprintf(valueText, sizeof(valueText), "%.1f", *m_value);

    const float right = m_bounds.x + m_bounds.width;
    const float buttonWidth = m_bounds.width / 8.0f;
    const float valueSpaceWidth = m_bounds.width / 4.0f;
    const float padding = 5.0f;
    const int textWidth = MeasureText(valueText, 20);

    DrawText(
        valueText,
        right - buttonWidth - padding - valueSpaceWidth / 2.0f - textWidth / 2.0f,
        m_bounds.y + m_bounds.height / 2.0f - 10.0f,
        20,
        WHITE
    );
}

} // namespace ui
