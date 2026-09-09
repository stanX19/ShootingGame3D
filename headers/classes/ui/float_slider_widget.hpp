#ifndef UI_FLOAT_SLIDER_WIDGET_HPP
#define UI_FLOAT_SLIDER_WIDGET_HPP

#include "includes.hpp"
#include "classes/ui/i_widget.hpp"
#include <string>

namespace ui {

class FloatSliderWidget : public IWidget {
public:
    FloatSliderWidget() = default;
    FloatSliderWidget(
        const std::string &label,
        float &valueRef,
        float minValue,
        float maxValue,
        float step,
        Rectangle bounds,
        Color color
    );

    void setBounds(Rectangle newBounds);

    bool update() override;
    void draw() override;

private:
    Rectangle getTrackBounds() const;
    float getKnobX(const Rectangle &trackBounds) const;
    float getSnappedValueFromMouseX(const Rectangle &trackBounds, float mouseX) const;

    std::string m_label;
    float *m_value = nullptr;
    float m_minValue = 0.01f;
    float m_maxValue = 1.0f;
    float m_step = 0.01f;
    Rectangle m_bounds = {0.0f, 0.0f, 0.0f, 0.0f};
    Color m_color = ORANGE;

    bool m_isDragging = false;
};

} // namespace ui

#endif // UI_FLOAT_SLIDER_WIDGET_HPP
