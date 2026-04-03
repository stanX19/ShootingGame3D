#include "classes/ui/text_button_widget.hpp"

namespace ui {

TextButtonWidget::TextButtonWidget(const std::string &text, Rectangle bounds, Color color, int fontSize)
    : text(text), bounds(bounds), color(color), fontSize(fontSize) {
}

void TextButtonWidget::setText(const std::string &newText) {
    text = newText;
}

void TextButtonWidget::setBounds(Rectangle newBounds) {
    bounds = newBounds;
}

void TextButtonWidget::setColor(Color newColor) {
    color = newColor;
}

bool TextButtonWidget::update() {
    hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
    down = hovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void TextButtonWidget::draw() {
    Color drawColor = ColorAlpha(color, 0.4f + (hovered + (hovered && down)) * 0.2f);

    DrawRectangleRec(bounds, drawColor);
    DrawRectangleLinesEx(bounds, 2, color);

    const int textWidth = MeasureText(text.c_str(), fontSize);
    DrawText(
        text.c_str(),
        bounds.x + bounds.width / 2 - textWidth / 2,
        bounds.y + bounds.height / 2 - fontSize / 2,
        fontSize,
        color
    );
}

} // namespace ui
