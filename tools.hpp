#pragma once
#include <SDL2/SDL.h>
#include "imgui/imgui.h"
#include <string>
#include <vector>
struct TextBox {
    std::string content = "Sample Text";
    SDL_Rect rect = {0, 0, 200, 50};
    int fontSize = 24;
    bool bold = false;
    bool italic = false;
    bool isActive = false;
    ImVec4 color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    int layerIndex = -1;
    std::string fontPath = "";

    // Simple equality check
    bool operator==(const TextBox& other) const {
        return content == other.content && rect.x == other.rect.x && rect.y == other.rect.y;
    }

    bool isEmpty() const {
        return content.empty();
    }
};

enum Tool {
    PENCIL, ERASER, LINE, FILL, RECT, CIRCLE, TRIANGLE, TEXT, SELECT, GRADIENT, NONE
};


enum GradientType { LINEAR, RADIAL, ANGULAR }; //https://icons8.medium.com/types-of-gradients-and-how-to-use-them-9508be01048d

namespace Tools {
    extern Tool currentTool;
    extern bool isDrawing;
    extern ImVec2 startPos, currentPos;
    extern ImVec4 currColor, secondaryColor;
    extern int brushSize, eraserSize;
    extern GradientType gradientType;
    extern std::vector<TextBox> textBoxes;
    extern int activeTextBox;


    void init();
    void cleanup();
    void handleSDLEvent(const SDL_Event& e);

    void drawLine(SDL_Renderer* rend, ImVec2 start, ImVec2 end, const ImVec4& color, int thickness = 1);
    void drawEraser(SDL_Renderer* rend, ImVec2 position, int size = 10);
    void drawRectangle(SDL_Renderer* rend, ImVec2 start, ImVec2 end, const ImVec4& color, bool filled);
    void drawCircle(SDL_Renderer* rend, ImVec2 center, int radius, const ImVec4& color, bool filled);
    void drawTriangle(SDL_Renderer* rend, ImVec2 p1, ImVec2 p2, ImVec2 p3, const ImVec4& color);
    void drawGradient(SDL_Renderer* rend, ImVec2 start, ImVec2 end, const ImVec4& color1, const ImVec4& color2, GradientType type);
    void floodFill(int x, int y, ImVec4 fillColor);

    // Text tool functions
    void createTextBox(int x, int y, int w, int h);
    void renderTextBoxes(SDL_Renderer* rend);
    void renderTextBox(SDL_Renderer* rend, const TextBox& textBox);

    // Some getter methods with inconsistent naming
    const std::vector<TextBox>& get_text_boxes();
    int getActiveTextBoxIndex();
    size_t textBoxCount();
}
