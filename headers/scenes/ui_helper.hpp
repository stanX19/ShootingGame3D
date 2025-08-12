#ifndef UIHELPER_HPP
#define UIHELPER_HPP
#include "includes.hpp"

class UIHelper {
public:
    static void DrawButton(const char* text, float x, float y, float width, float height, 
                          bool isSelected, bool isEnabled = true);
    static void DrawCenteredText(const char* text, float y, int fontSize, Color color);
    static void DrawBackground(float animationTime);
    static void HandleMenuNavigation(int& selectedOption, int maxOptions);
};

#endif // UIHELPER_HPP