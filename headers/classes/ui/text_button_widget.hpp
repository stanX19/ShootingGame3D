#ifndef UI_TEXT_BUTTON_WIDGET_HPP
#define UI_TEXT_BUTTON_WIDGET_HPP

#include "includes.hpp"
#include "classes/ui/i_widget.hpp"
#include <string>

namespace ui {

class TextButtonWidget : public IWidget {
public:
    TextButtonWidget() = default;
    TextButtonWidget(const std::string &text, Rectangle bounds, Color color, int fontSize = 20);

    void setText(const std::string &newText);
    void setBounds(Rectangle newBounds);
    void setColor(Color newColor);

    bool update() override;
    void draw() override;

private:
    std::string text;
    Rectangle bounds = {0.0f, 0.0f, 0.0f, 0.0f};
    Color color = WHITE;
    int fontSize = 20;

    bool hovered = false;
    bool down = false;
};

} // namespace ui

#endif // UI_TEXT_BUTTON_WIDGET_HPP
