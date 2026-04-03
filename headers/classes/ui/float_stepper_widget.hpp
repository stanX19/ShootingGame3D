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
        float step,
        float minValue,
        float maxValue,
        Rectangle bounds,
        Color color
    );

    void setBounds(Rectangle newBounds);

    bool update() override;
    void draw() override;

private:
    void syncChildBounds();

    std::string label;
    float *value = nullptr;
    float step = 0.1f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    Rectangle bounds = {0.0f, 0.0f, 0.0f, 0.0f};
    Color color = SKYBLUE;

    TextButtonWidget minusButton;
    TextButtonWidget plusButton;
};

} // namespace ui

#endif // UI_FLOAT_STEPPER_WIDGET_HPP
