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
    : m_label(label), m_value(&valueRef), m_minValue(minValue), m_maxValue(maxValue), m_step(step), m_bounds(bounds), m_color(color) {
}

void FloatSliderWidget::setBounds(Rectangle newBounds) {
    m_bounds = newBounds;
}

Rectangle FloatSliderWidget::getTrackBounds() const {
    const float sliderLeft = m_bounds.x + m_bounds.width / 2.0f + 5.0f;
    const float sliderRight = m_bounds.x + m_bounds.width - 10.0f;
    const float sliderWidth = std::max(1.0f, sliderRight - sliderLeft);

    return Rectangle{
        sliderLeft,
        m_bounds.y + m_bounds.height / 2.0f - 3.0f,
        sliderWidth,
        6.0f
    };
}

float FloatSliderWidget::getKnobX(const Rectangle &trackBounds) const {
    if (!m_value || m_maxValue <= m_minValue) {
        return trackBounds.x;
    }

    const float clampedValue = Clamp(*m_value, m_minValue, m_maxValue);
    const float normalized = (clampedValue - m_minValue) / (m_maxValue - m_minValue);
    return trackBounds.x + trackBounds.width * normalized;
}

float FloatSliderWidget::getSnappedValueFromMouseX(const Rectangle &trackBounds, float mouseX) const {
    if (m_maxValue <= m_minValue) {
        return m_minValue;
    }

    const float rawNormalized = (mouseX - trackBounds.x) / trackBounds.width;
    const float clampedNormalized = Clamp(rawNormalized, 0.0f, 1.0f);
    const float rawValue = m_minValue + (m_maxValue - m_minValue) * clampedNormalized;

    return Clamp(std::round(rawValue / m_step) * m_step, m_minValue, m_maxValue);
}

bool FloatSliderWidget::update() {
    if (!m_value) {
        return false;
    }

    *m_value = Clamp(*m_value, m_minValue, m_maxValue);
    const Rectangle trackBounds = getTrackBounds();
    const Rectangle interactionBounds = {
        trackBounds.x,
        m_bounds.y,
        trackBounds.width,
        m_bounds.height
    };

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)
        && CheckCollisionPointRec(GetMousePosition(), interactionBounds)) {
        m_isDragging = true;
    }

    bool changed = false;

    if (m_isDragging && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        const float snappedValue = getSnappedValueFromMouseX(trackBounds, GetMousePosition().x);
        if (std::abs(snappedValue - *m_value) > 0.0001f) {
            *m_value = snappedValue;
            changed = true;
        }
    }

    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        m_isDragging = false;
    }

    return changed;
}

void FloatSliderWidget::draw() {
    if (!m_value) {
        return;
    }

    DrawRectangleLinesEx(m_bounds, 2.0f, ColorAlpha(m_color, 0.5f));
    DrawText(m_label.c_str(), m_bounds.x + 10.0f, m_bounds.y + m_bounds.height / 2.0f - 10.0f, 20, m_color);

    const Rectangle trackBounds = getTrackBounds();
    DrawRectangleRec(trackBounds, ColorAlpha(m_color, 0.35f));

    const float knobX = getKnobX(trackBounds);
    const Rectangle knob = {
        knobX - 6.0f,
        m_bounds.y + 6.0f,
        12.0f,
        m_bounds.height - 12.0f
    };
    DrawRectangleRec(knob, ColorAlpha(m_color, m_isDragging ? 1.0f : 0.85f));
}

} // namespace ui
