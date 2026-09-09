#ifndef UI_FLOAT_STEPPER_WIDGET_HPP
#define UI_FLOAT_STEPPER_WIDGET_HPP

#include "includes.hpp"
#include "classes/ui/i_widget.hpp"
#include "classes/ui/text_button_widget.hpp"
#include <string>

namespace ui {

class FloatStepperWidget : public IWidget {
public:
    FloatStepperWidget() = default;
    FloatStepperWidget(
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
    void syncChildBounds();

    std::string m_label;
    float *m_value = nullptr;
    float m_step = 0.1f;
    float m_minValue = 0.0f;
    float m_maxValue = 1.0f;
    Rectangle m_bounds = {0.0f, 0.0f, 0.0f, 0.0f};
    Color m_color = SKYBLUE;

    TextButtonWidget m_minusButton;
    TextButtonWidget m_plusButton;
};

} // namespace ui

#endif // UI_FLOAT_STEPPER_WIDGET_HPP
