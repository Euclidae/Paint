#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>
#include <map>
#include "imgui/imgui.h"
#include "tools.hpp"

extern int CANVAS_WIDTH;
extern int CANVAS_HEIGHT;
constexpr int TUI_WIDTH = 300;

struct TextState;

namespace Canvas {
    struct Layer {
        SDL_Texture* texture = nullptr;
        std::string name;
        float opacity = 1.0f;
        bool visible = true;
        bool locked = false;
        int blendMode = 0;
        bool selected = false;
        bool beingDragged = false;
    };

    extern std::vector<Layer> layers;
    extern int activeLayerIndex;
    extern SDL_Texture* canvasBuffer;
    extern SDL_Renderer* renderer;
    extern std::map<int, TTF_Font*> fontCache;

    void init(SDL_Renderer* rend);
    void cleanup();
    void setupNewCanvas(int width, int height);
    void addLayer(const std::string& name = "New Layer", bool isTextLayer = false);
    void duplicateLayer(int index);
    void removeLayer(int index);
    void moveLayer(int fromIndex, int toIndex);
    void createNewLayerTexture(Layer& layer);

    void render();
    void clearFontCache();
    TTF_Font* getFont(int size, bool bold, bool italic);
    void importImage(const char* filePath);
    void exportImage(const char* filePath, const char* format);
}

extern SDL_Rect selectionRect;
extern bool hasSelection;
extern SDL_Texture* selectionTexture;
extern ImVec2 selectionOffset;
