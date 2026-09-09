#include "classes/ui/text_button_widget.hpp"

namespace ui {

TextButtonWidget::TextButtonWidget(const std::string &text, Rectangle bounds, Color color, int fontSize)
    : m_text(text), m_bounds(bounds), m_color(color), m_fontSize(fontSize) {
}

void TextButtonWidget::setText(const std::string &newText) {
    m_text = newText;
}

void TextButtonWidget::setBounds(Rectangle newBounds) {
    m_bounds = newBounds;
}

void TextButtonWidget::setColor(Color newColor) {
    m_color = newColor;
}

bool TextButtonWidget::update() {
    m_hovered = CheckCollisionPointRec(GetMousePosition(), m_bounds);
    m_down = m_hovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    return m_hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void TextButtonWidget::draw() {
    Color drawColor = ColorAlpha(m_color, 0.4f + (m_hovered + (m_hovered && m_down)) * 0.2f);

    DrawRectangleRec(m_bounds, drawColor);
    DrawRectangleLinesEx(m_bounds, 2, m_color);

    const int textWidth = MeasureText(m_text.c_str(), m_fontSize);
    DrawText(
        m_text.c_str(),
        m_bounds.x + m_bounds.width / 2 - textWidth / 2,
        m_bounds.y + m_bounds.height / 2 - m_fontSize / 2,
        m_fontSize,
        m_color
    );
}

} // namespace ui
