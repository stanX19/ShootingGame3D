#ifndef UI_I_WIDGET_HPP
#define UI_I_WIDGET_HPP

namespace ui {

class IWidget {
public:
    virtual ~IWidget() = default;

    bool tick_and_draw() {
        const bool changed = update();
        draw();
        return changed;
    }

    virtual bool update() = 0;
    virtual void draw() = 0;
};

} // namespace ui

#endif // UI_I_WIDGET_HPP
