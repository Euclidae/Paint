// TODO Finally replace that pesky TUI with a proper UI
// DearImGUI was chosen. Thanks to its docs and thanks to TheCherno
// This was all put in a single file because Euclidae is too lazy to refactor. Will do so some other day.
// I always forget if Windows needs this or not... better safe than sorry
// N.B Please be warned that this code is a mess and needs refactoring. And please understand that this is a work in progress.
// Here be memory leaks and seg faults, tread with caution.
// TODO : Fix eraser button
// TODO : Implement gradient tool
// TODO : Add undo/redo functionality
//
#include <SDL2/SDL_blendmode.h>
#ifdef _WIN32
    #define SDL_MAIN_HANDLED
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "tinyfiledialogs/tinyfiledialogs.h" // SDL3 would have been much better.
#include <vector>
#include <functional>
#include <iostream>
#include <stack>
#include <algorithm>
#include <string>
#include <memory>
#include <cmath>
#include <map>
#include <cctype>
#include <queue>
#include <fstream>

// Canvas dimensions (fixed size for UI consistency)
int CANVAS_WIDTH = 1280;
int CANVAS_HEIGHT = 720;
const int TUI_WIDTH = 300;  // Width of the side panel

// Tool states - made these exclusive
enum Tool {
    PENCIL,
    ERASER,
    LINE,
    FILL,
    RECT,
    CIRCLE,
    TRIANGLE,
    TEXT,
    SELECT,
    GRADIENT,
    NONE
};
Tool currentTool = NONE;

// Drawing states
bool isDrawing = false;
ImVec2 startPos;  // Track starting point for shapes
ImVec2 currentPos;  // Track current mouse position

// Current color (RGB 0-255)
ImVec4 currColor(0.0f, 0.0f, 0.0f, 1.0f);  // Start with black
ImVec4 secondaryColor(1.0f, 1.0f, 1.0f, 1.0f); // Secondary color for gradient tool
int brushSize = 5; // Default brush size
int eraserSize = 20; // Default eraser size

// Font properties for text tool
std::string textContent = "Hello World";
int fontSize = 24;
bool textBold = false;
bool textItalic = false;

// Layer management
struct Layer {
    SDL_Texture* texture;
    std::string name;
    float opacity = 1.0f;
    bool visible = true;
    bool locked = false; // Prevent modifications
    int blendMode = 0;  // 0 = normal, 1 = multiply, 2 = add, 3 = screen
    bool selected = false; // For UI selection
    bool beingDragged = false;  // For UI reordering
};

std::vector<Layer> layers;
int activeLayerIndex = 0;
SDL_Texture* canvasBuffer = nullptr;  // Main drawing buffer

// Undo/redo system
struct HistoryState {
    SDL_Texture* texture;
    int layerIndex;
};
std::stack<HistoryState> undoStack;
std::stack<HistoryState> redoStack;

// Selection state
SDL_Rect selectionRect = {0, 0, 0, 0};
bool hasSelection = false;
SDL_Texture* selectionTexture = nullptr;
ImVec2 selectionOffset;

// Gradient tool state
enum GradientType { LINEAR, RADIAL, ANGULAR };
GradientType gradientType = LINEAR;

// SDL objects
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;
std::map<int, TTF_Font*> fontCache; // Cache for different font sizes

// Function declarations
void setupNewCanvas(int width, int height);
void importImage(const char* filePath);
void exportImage(const char* filePath, const char* format);
void handleKeyboard(SDL_Keycode key);
void handleMouseClick(int button, int state, int x, int y);
void handleMouseMotion(int x, int y);
void renderUI();
void renderCanvas();
void addLayer(const std::string& name = "New Layer");
void duplicateLayer(int index);
void removeLayer(int index);
void setActiveLayer(int index);
void updateLayerOpacity(int index, float opacity);
void updateLayerVisibility(int index, bool visible);
void changeBlendMode(int index, int mode);
void moveLayer(int fromIndex, int toIndex);
void createNewLayerTexture(Layer& layer);
void renderToTexture(SDL_Texture* target);
void drawLine(SDL_Renderer* rend, ImVec2 start, ImVec2 end, const ImVec4& color, int thickness = 1);
void drawEraser(SDL_Renderer* rend, ImVec2 position, int size = 10);
void drawRectangle(SDL_Renderer* rend, ImVec2 start, ImVec2 end, const ImVec4& color, bool filled);
void drawCircle(SDL_Renderer* rend, ImVec2 center, int radius, const ImVec4& color, bool filled);
void drawTriangle(SDL_Renderer* rend, ImVec2 p1, ImVec2 p2, ImVec2 p3, const ImVec4& color);
void drawText(SDL_Renderer* rend, int x, int y, const std::string& text, const ImVec4& color);
void drawGradient(SDL_Renderer* rend, ImVec2 start, ImVec2 end, const ImVec4& color1, const ImVec4& color2, GradientType type);
void floodFill(int x, int y, ImVec4 fillColor);
void handleFileMenu();
void saveUndoState();
void applyUndo();
void applyRedo();
void clearSelection();
void copySelection();
void pasteSelection();
void deleteSelection();
void mergeLayers();
TTF_Font* getFont(int size, bool bold, bool italic);

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL could not initialize! Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image could not initialize! Error: " << IMG_GetError() << std::endl;
        return 1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! Error: " << TTF_GetError() << std::endl;
        return 1;
    }

    // Create window
    window = SDL_CreateWindow("Advanced Image Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             CANVAS_WIDTH + TUI_WIDTH, CANVAS_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Window could not be created! Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer could not be created! Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // Set up initial canvas
    setupNewCanvas(CANVAS_WIDTH, CANVAS_HEIGHT);
    addLayer("Background");

    // Main loop flag
    bool quit = false;
    SDL_Event e;

    // Main loop
    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);

            switch (e.type) {
                case SDL_QUIT:
                    quit = true;
                    break;

                case SDL_KEYDOWN:
                    handleKeyboard(e.key.keysym.sym);
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    handleMouseClick(e.button.button, SDL_PRESSED, e.button.x, e.button.y);
                    break;

                case SDL_MOUSEBUTTONUP:
                    handleMouseClick(e.button.button, SDL_RELEASED, e.button.x, e.button.y);
                    break;

                case SDL_MOUSEMOTION:
                    handleMouseMotion(e.motion.x, e.motion.y);
                    break;

                case SDL_MOUSEWHEEL:
                    // Zoom with mouse wheel
                    if (e.wheel.y > 0) brushSize = std::min(50, brushSize + 1);
                    else if (e.wheel.y < 0) brushSize = std::max(1, brushSize - 1);
                    break;

                case SDL_WINDOWEVENT:
                    if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                        // Handle window resize if needed
                    }
                    break;
            }
        }

        // Start ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Render UI
        renderUI();

        // Render canvas
        renderCanvas();

        // Render ImGui
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

        // Update screen
        SDL_RenderPresent(renderer);

        // Cap frame rate
        SDL_Delay(16);
    }

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    for (auto& layer : layers) {
        if (layer.texture) SDL_DestroyTexture(layer.texture);
    }
    if (canvasBuffer) SDL_DestroyTexture(canvasBuffer);
    if (selectionTexture) SDL_DestroyTexture(selectionTexture);

    for (auto& fontPair : fontCache) {
        TTF_CloseFont(fontPair.second);
    }
    fontCache.clear();

    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}

TTF_Font* getFont(int size, bool bold, bool italic) {
    int key = (size << 2) | (bold ? 1 : 0) | (italic ? 2 : 0);
    if (fontCache.find(key) != fontCache.end()) {
        return fontCache[key];
    }

    TTF_Font* font = TTF_OpenFont("arial.ttf", size);
    if (!font) {
        font = TTF_OpenFont("DejaVuSans.ttf", size);
        if (!font) {
            std::cerr << "Failed to load font!" << std::endl;
            return nullptr;
        }
    }

    int style = TTF_STYLE_NORMAL;
    if (bold) style |= TTF_STYLE_BOLD;
    if (italic) style |= TTF_STYLE_ITALIC;
    TTF_SetFontStyle(font, style);

    fontCache[key] = font;
    return font;
}

void createNewLayerTexture(Layer& layer) {
    if (layer.texture) SDL_DestroyTexture(layer.texture);
    layer.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                     SDL_TEXTUREACCESS_TARGET,
                                     CANVAS_WIDTH, CANVAS_HEIGHT);
    SDL_SetTextureBlendMode(layer.texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, layer.texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, nullptr);
}

void setupNewCanvas(int width, int height) {
    CANVAS_WIDTH = width;
    CANVAS_HEIGHT = height;

    // Recreate canvas buffer
    if (canvasBuffer) SDL_DestroyTexture(canvasBuffer);
    canvasBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET, CANVAS_WIDTH, CANVAS_HEIGHT);
    SDL_SetTextureBlendMode(canvasBuffer, SDL_BLENDMODE_BLEND);

    // Recreate all layer textures
    for (auto& layer : layers) {
        createNewLayerTexture(layer);
    }

    // Clear undo/redo history
    while (!undoStack.empty()) undoStack.pop();
    while (!redoStack.empty()) redoStack.pop();
}

void importImage(const char* filePath) {
    SDL_Surface* loadedSurface = IMG_Load(filePath);
    if (!loadedSurface) {
        std::cerr << "IMG_Load failed: " << IMG_GetError() << std::endl;
        return;
    }

    // Create new layer for image
    addLayer("Imported");
    Layer& layer = layers[activeLayerIndex];

    // Create texture from surface
    SDL_Texture* imgTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
    if (!imgTexture) {
        std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(loadedSurface);
        return;
    }

    // Render to layer texture
    SDL_SetRenderTarget(renderer, layer.texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    // Get image dimensions
    int imgWidth, imgHeight;
    SDL_QueryTexture(imgTexture, nullptr, nullptr, &imgWidth, &imgHeight);

    // Scale image to fit canvas without resizing window
    SDL_Rect destRect = {0, 0, CANVAS_WIDTH, CANVAS_HEIGHT};
    SDL_RenderCopy(renderer, imgTexture, nullptr, &destRect);

    SDL_SetRenderTarget(renderer, nullptr);
    SDL_DestroyTexture(imgTexture);
    SDL_FreeSurface(loadedSurface);

    // Save undo state
    saveUndoState();
}

void exportImage(const char* filePath, const char* format) {
    // Create surface to hold final image
    // Thanks John Purcell for his particle explosion tutorial. It is where I picked up the idea of using SDL_CreateRGBSurface.
    SDL_Surface* surface = SDL_CreateRGBSurface(0, CANVAS_WIDTH, CANVAS_HEIGHT, 32,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

    if (!surface) {
        std::cerr << "Failed to create surface: " << SDL_GetError() << std::endl;
        return;
    }

    // Set render target to canvas buffer and read pixels
    SDL_SetRenderTarget(renderer, canvasBuffer);
    SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch);
    SDL_SetRenderTarget(renderer, nullptr);

    // Save based on format
    std::string fileStr(filePath);
    std::string formatStr(format);
    std::transform(formatStr.begin(), formatStr.end(), formatStr.begin(), ::tolower);

    if (formatStr == "png") {
        IMG_SavePNG(surface, filePath);
    } else if (formatStr == "jpg" || formatStr == "jpeg") {
        IMG_SaveJPG(surface, filePath, 90); // 90% quality
    } else if (formatStr == "bmp") {
        SDL_SaveBMP(surface, filePath);
    } else {
        std::cerr << "Unsupported format: " << format << std::endl;
    }

    SDL_FreeSurface(surface);
}

void renderCanvas() {
    // Render to canvas buffer
    SDL_SetRenderTarget(renderer, canvasBuffer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // Composite all visible layers
    for (auto& layer : layers) {
        if (!layer.visible) continue;

        SDL_SetTextureAlphaMod(layer.texture, (Uint8)(layer.opacity * 255));
        switch (layer.blendMode) {
            case 1: SDL_SetTextureBlendMode(layer.texture, SDL_BLENDMODE_MOD); break;
            case 2: SDL_SetTextureBlendMode(layer.texture, SDL_BLENDMODE_ADD); break;
            case 3: SDL_SetTextureBlendMode(layer.texture, SDL_BLENDMODE_INVALID); break;
            default: SDL_SetTextureBlendMode(layer.texture, SDL_BLENDMODE_BLEND);
        }

        SDL_RenderCopy(renderer, layer.texture, nullptr, nullptr);
    }

    // Draw preview for current tool
    if (isDrawing) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        switch (currentTool) {
            case LINE:
                drawLine(renderer, startPos, currentPos, currColor, brushSize);
                break;

            case RECT:
                drawRectangle(renderer, startPos, currentPos, currColor, false);
                break;

            case CIRCLE: {
                int radius = static_cast<int>(sqrt(pow(currentPos.x - startPos.x, 2) +
                                            pow(currentPos.y - startPos.y, 2)));
                drawCircle(renderer, startPos, radius, currColor, false);
                break;
            }

            case TRIANGLE: {
                ImVec2 p3 = ImVec2(startPos.x, currentPos.y);
                drawTriangle(renderer, startPos, currentPos, p3, currColor);
                break;
            }

            case GRADIENT:
                drawGradient(renderer, startPos, currentPos, currColor, secondaryColor, gradientType);
                break;

            case PENCIL:
                drawLine(renderer, startPos, currentPos, currColor, brushSize);
                startPos = currentPos; // For continuous drawing
                break;

            case ERASER:
                drawEraser(renderer, currentPos, eraserSize);
                break;

            case SELECT:
                // Draw selection rectangle
                SDL_SetRenderDrawColor(renderer, 100, 200, 255, 150);
                SDL_Rect selectRect = {
                    (int)std::min(startPos.x, currentPos.x),
                    (int)std::min(startPos.y, currentPos.y),
                    (int)std::abs(currentPos.x - startPos.x),
                    (int)std::abs(currentPos.y - startPos.y)
                };
                SDL_RenderDrawRect(renderer, &selectRect);

                // Draw dashed outline
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                for (int i = 0; i < selectRect.w; i += 5) {
                    if (i % 10 < 5) {
                        SDL_RenderDrawPoint(renderer, selectRect.x + i, selectRect.y);
                        SDL_RenderDrawPoint(renderer, selectRect.x + i, selectRect.y + selectRect.h - 1);
                    }
                }
                for (int i = 0; i < selectRect.h; i += 5) {
                    if (i % 10 < 5) {
                        SDL_RenderDrawPoint(renderer, selectRect.x, selectRect.y + i);
                        SDL_RenderDrawPoint(renderer, selectRect.x + selectRect.w - 1, selectRect.y + i);
                    }
                }
                break;
        }
    }

    // Draw selection if exists
    if (hasSelection && selectionTexture) {
        SDL_Rect destRect = {
            selectionRect.x + (int)selectionOffset.x,
            selectionRect.y + (int)selectionOffset.y,
            selectionRect.w,
            selectionRect.h
        };
        SDL_RenderCopy(renderer, selectionTexture, nullptr, &destRect);

        // Draw selection outline
        SDL_SetRenderDrawColor(renderer, 100, 200, 255, 200);
        SDL_RenderDrawRect(renderer, &destRect);
    }

    // Reset render target
    SDL_SetRenderTarget(renderer, nullptr);

    // Render to screen
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderClear(renderer);

    // Draw canvas with border
    SDL_Rect canvasRect = {10, 10, CANVAS_WIDTH, CANVAS_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &canvasRect);

    canvasRect.x += 1;
    canvasRect.y += 1;
    canvasRect.w -= 2;
    canvasRect.h -= 2;
    SDL_RenderCopy(renderer, canvasBuffer, nullptr, &canvasRect);
}

void renderUI() {
    // File menu
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Canvas")) {
                setupNewCanvas(1280, 720);
            }
            if (ImGui::MenuItem("Open Image")) {
                const char* filePath = tinyfd_openFileDialog(
                    "Open Image File",
                    "",
                    0,
                    nullptr,
                    "Image files",
                    0
                );
                if (filePath) importImage(filePath);
            }
            if (ImGui::BeginMenu("Save As")) {
                if (ImGui::MenuItem("PNG")) {
                    const char* filePath = tinyfd_saveFileDialog(
                        "Save Image as PNG",
                        "untitled.png",
                        0,
                        nullptr,
                        "PNG files"
                    );
                    if (filePath) exportImage(filePath, "png");
                }
                if (ImGui::MenuItem("JPEG")) {
                    const char* filePath = tinyfd_saveFileDialog(
                        "Save Image as JPEG",
                        "untitled.jpg",
                        0,
                        nullptr,
                        "JPEG files"
                    );
                    if (filePath) exportImage(filePath, "jpg");
                }
                if (ImGui::MenuItem("BMP")) {
                    const char* filePath = tinyfd_saveFileDialog(
                        "Save Image as BMP",
                        "untitled.bmp",
                        0,
                        nullptr,
                        "BMP files"
                    );
                    if (filePath) exportImage(filePath, "bmp");
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Exit")) {
                SDL_Event quitEvent;
                quitEvent.type = SDL_QUIT;
                SDL_PushEvent(&quitEvent);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                applyUndo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                applyRedo();
            }
            if (ImGui::MenuItem("Copy Selection", "Ctrl+C", false, hasSelection)) {
                copySelection();
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                pasteSelection();
            }
            if (ImGui::MenuItem("Delete Selection", "Del", false, hasSelection)) {
                deleteSelection();
            }
            if (ImGui::MenuItem("Clear Selection", "Esc", false, hasSelection)) {
                clearSelection();
            }
            if (ImGui::MenuItem("Merge Layers", "Ctrl+M")) {
                mergeLayers();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                ImGui::OpenPopup("About");
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // About popup
    if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Advanced Image Editor");
        ImGui::Text("Version 1.0");
        ImGui::Text("Created with SDL2, Dear ImGui, and TinyFileDialogs");
        ImGui::Separator();
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Tools panel
    ImGui::SetNextWindowPos(ImVec2(CANVAS_WIDTH + 20, 20));
    ImGui::SetNextWindowSize(ImVec2(TUI_WIDTH - 30, CANVAS_HEIGHT - 40));
    ImGui::Begin("Tools & Properties", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // Tool selection
    ImGui::Text("Drawing Tools");
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

    if (ImGui::Button("Pencil", ImVec2(60, 30))) currentTool = PENCIL;
    ImGui::SameLine();
    if (ImGui::Button("Eraser", ImVec2(60, 30))) currentTool = ERASER;
    ImGui::SameLine();
    if (ImGui::Button("Line", ImVec2(60, 30))) currentTool = LINE;

    if (ImGui::Button("Rectangle", ImVec2(60, 30))) currentTool = RECT;
    ImGui::SameLine();
    if (ImGui::Button("Circle", ImVec2(60, 30))) currentTool = CIRCLE;
    ImGui::SameLine();
    if (ImGui::Button("Triangle", ImVec2(60, 30))) currentTool = TRIANGLE;

    if (ImGui::Button("Fill", ImVec2(60, 30))) currentTool = FILL;
    ImGui::SameLine();
    if (ImGui::Button("Text", ImVec2(60, 30))) currentTool = TEXT;
    ImGui::SameLine();
    if (ImGui::Button("Select", ImVec2(60, 30))) currentTool = SELECT;

    if (ImGui::Button("Gradient", ImVec2(60, 30))) currentTool = GRADIENT;
    ImGui::SameLine();
    if (ImGui::Button("None", ImVec2(60, 30))) currentTool = NONE;

    ImGui::PopStyleVar();

    // Tool properties
    ImGui::Separator();
    ImGui::Text("Tool Properties");

    if (currentTool == PENCIL || currentTool == ERASER || currentTool == LINE) {
        if (currentTool == ERASER) {
            ImGui::SliderInt("Eraser Size", &eraserSize, 1, 100);
        } else {
            ImGui::SliderInt("Brush Size", &brushSize, 1, 50);
        }
    }

    if (currentTool == TEXT) {
        static char textBuffer[256] = "";  // Static buffer persists between frames

        // Initialize buffer from textContent if empty
        if (strlen(textBuffer) == 0 && !textContent.empty()) {
            strncpy(textBuffer, textContent.c_str(), sizeof(textBuffer) - 1);
            textBuffer[sizeof(textBuffer) - 1] = '\0';  // Ensure null termination
        }

        // Update textContent when user types
        if (ImGui::InputText("Text", textBuffer, sizeof(textBuffer))) {
            textContent = std::string(textBuffer);
        }

        ImGui::SliderInt("Font Size", &fontSize, 8, 72);
        ImGui::Checkbox("Bold", &textBold);
        ImGui::SameLine();
        ImGui::Checkbox("Italic", &textItalic);
    }

    if (currentTool == GRADIENT) {
        const char* gradientTypes[] = { "Linear", "Radial", "Angular" };
        ImGui::Combo("Gradient Type", (int*)&gradientType, gradientTypes, 3);
    }

    // Color pickers
    ImGui::Separator();
    ImGui::Text("Colors");

    ImGui::ColorEdit3("Primary", (float*)&currColor);
    if (currentTool == GRADIENT) {
        ImGui::ColorEdit3("Secondary", (float*)&secondaryColor);
    }

    // Layer management
    ImGui::Separator();
    ImGui::Text("Layers");

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    if (ImGui::Button("+ Add", ImVec2(60, 25))) {
        addLayer();
    }
    ImGui::SameLine();
    if (ImGui::Button("Duplicate", ImVec2(80, 25)) && !layers.empty()) {
        duplicateLayer(activeLayerIndex);
    }
    ImGui::SameLine();
    if (ImGui::Button("- Remove", ImVec2(80, 25)) && layers.size() > 1) {
        removeLayer(activeLayerIndex);
    }
    ImGui::PopStyleVar();

    ImGui::BeginChild("LayerList", ImVec2(0, 200), true);
    for (int i = 0; i < layers.size(); i++) {
        ImGui::PushID(i);  // This should make all child elements unique

        bool isActiveLayer = (i == activeLayerIndex);

        // Highlight active layer
        if (isActiveLayer) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        }

        // Layer name button
        if (ImGui::Button(layers[i].name.c_str(), ImVec2(100, 0))) {
            activeLayerIndex = i;
        }

        // Always pop the style color if we pushed it
        if (isActiveLayer) {
            ImGui::PopStyleColor();
        }

        // Drag and drop source
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("LAYER_DRAG", &i, sizeof(int));
            ImGui::Text("Moving %s", layers[i].name.c_str());
            ImGui::EndDragDropSource();
        }

        // Drag and drop target
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LAYER_DRAG")) {
                int sourceIndex = *(const int*)payload->Data;
                moveLayer(sourceIndex, i);
            }
            ImGui::EndDragDropTarget();
        }

        // Visibility toggle - use explicit unique IDs
        ImGui::SameLine();
        ImGui::PushID("visible");
        ImGui::Checkbox("##vis", &layers[i].visible);
        ImGui::PopID();

        // Lock toggle - use explicit unique IDs
        ImGui::SameLine();
        ImGui::PushID("lock");
        ImGui::Checkbox("##lock", &layers[i].locked);
        ImGui::PopID();

        // Opacity slider - use explicit unique IDs
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::PushID("opacity");
        ImGui::SliderFloat("##opacity", &layers[i].opacity, 0.0f, 1.0f, "%.2f");
        ImGui::PopID();

        ImGui::PopID();  // Pop the layer index ID
    }
    ImGui::EndChild();

    // Layer blend modes
    if (!layers.empty()) {
        ImGui::Separator();
        ImGui::Text("Blend Mode");
        const char* blendModes[] = {"Normal", "Multiply", "Add", "Screen"};
        ImGui::Combo("##BlendMode", &layers[activeLayerIndex].blendMode, blendModes, 4);
    }

    // Canvas properties
    ImGui::Separator();
    ImGui::Text("Canvas Size: %d x %d", CANVAS_WIDTH, CANVAS_HEIGHT);
    static int newWidth = CANVAS_WIDTH;
    static int newHeight = CANVAS_HEIGHT;
    ImGui::InputInt("Width", &newWidth);
    ImGui::InputInt("Height", &newHeight);
    newWidth = std::max(100, newWidth);
    newHeight = std::max(100, newHeight);

    if (ImGui::Button("Resize Canvas")) {
        setupNewCanvas(newWidth, newHeight);
    }

    ImGui::End();
}

void addLayer(const std::string& name) {
    Layer newLayer;
    newLayer.name = name;
    createNewLayerTexture(newLayer);
    layers.push_back(newLayer);
    activeLayerIndex = layers.size() - 1;
    saveUndoState();
}

void duplicateLayer(int index) {
    if (index < 0 || index >= layers.size()) return;

    Layer dupLayer = layers[index];
    dupLayer.name += " Copy";
    createNewLayerTexture(dupLayer);

    // Copy content
    SDL_SetRenderTarget(renderer, dupLayer.texture);
    SDL_RenderCopy(renderer, layers[index].texture, nullptr, nullptr);
    SDL_SetRenderTarget(renderer, nullptr);

    layers.insert(layers.begin() + index + 1, dupLayer);
    activeLayerIndex = index + 1;
    saveUndoState();
}

void removeLayer(int index) {
    if (index < 0 || index >= layers.size()) return;

    if (layers[index].texture) {
        SDL_DestroyTexture(layers[index].texture);
    }

    layers.erase(layers.begin() + index);

    if (activeLayerIndex >= layers.size()) {
        activeLayerIndex = layers.size() - 1;
    }
    saveUndoState();
}

void moveLayer(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= layers.size() ||
        toIndex < 0 || toIndex >= layers.size() || fromIndex == toIndex) return;

    Layer movedLayer = layers[fromIndex];
    layers.erase(layers.begin() + fromIndex);

    if (toIndex > fromIndex) toIndex--;
    layers.insert(layers.begin() + toIndex, movedLayer);

    // Update active layer index
    if (activeLayerIndex == fromIndex) {
        activeLayerIndex = toIndex;
    } else if (fromIndex < toIndex) {
        if (activeLayerIndex > fromIndex && activeLayerIndex <= toIndex) {
            activeLayerIndex--;
        }
    } else {
        if (activeLayerIndex >= toIndex && activeLayerIndex < fromIndex) {
            activeLayerIndex++;
        }
    }
    saveUndoState();
}

void saveUndoState() {
    if (activeLayerIndex < 0 || activeLayerIndex >= layers.size()) return;

    // Create a copy of the current layer state
    SDL_Texture* copy = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_TARGET,
                                         CANVAS_WIDTH, CANVAS_HEIGHT);
    SDL_SetRenderTarget(renderer, copy);
    SDL_RenderCopy(renderer, layers[activeLayerIndex].texture, nullptr, nullptr);
    SDL_SetRenderTarget(renderer, nullptr);

    // Save to undo stack
    HistoryState state;
    state.texture = copy;
    state.layerIndex = activeLayerIndex;
    undoStack.push(state);

    // Clear redo stack
    while (!redoStack.empty()) {
        SDL_DestroyTexture(redoStack.top().texture);
        redoStack.pop();
    }
}

void applyUndo() {
    if (undoStack.empty()) return;

    // Save current state to redo
    HistoryState currentState;
    currentState.texture = layers[undoStack.top().layerIndex].texture;
    currentState.layerIndex = undoStack.top().layerIndex;
    redoStack.push(currentState);

    // Apply undo state
    HistoryState undoState = undoStack.top();
    undoStack.pop();

    activeLayerIndex = undoState.layerIndex;
    layers[activeLayerIndex].texture = undoState.texture;
}

void applyRedo() {
    if (redoStack.empty()) return;

    // Save current state to undo
    HistoryState currentState;
    currentState.texture = layers[redoStack.top().layerIndex].texture;
    currentState.layerIndex = redoStack.top().layerIndex;
    undoStack.push(currentState);

    // Apply redo state
    HistoryState redoState = redoStack.top();
    redoStack.pop();

    activeLayerIndex = redoState.layerIndex;
    layers[activeLayerIndex].texture = redoState.texture;
}

void clearSelection() {
    if (selectionTexture) {
        SDL_DestroyTexture(selectionTexture);
        selectionTexture = nullptr;
    }
    hasSelection = false;
}

void copySelection() {
    if (!hasSelection) return;

    // Create texture for selection
    if (selectionTexture) SDL_DestroyTexture(selectionTexture);
    selectionTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_TARGET,
                                       selectionRect.w, selectionRect.h);

    // Copy selected area
    SDL_SetRenderTarget(renderer, selectionTexture);
    SDL_Rect srcRect = {selectionRect.x, selectionRect.y, selectionRect.w, selectionRect.h};
    SDL_RenderCopy(renderer, layers[activeLayerIndex].texture, &srcRect, nullptr);
    SDL_SetRenderTarget(renderer, nullptr);
}

void pasteSelection() {
    if (!selectionTexture) return;

    // Create a new layer for pasted content
    addLayer("Pasted");

    // Render selection to new layer
    SDL_SetRenderTarget(renderer, layers[activeLayerIndex].texture);
    SDL_Rect destRect = {0, 0, selectionRect.w, selectionRect.h};
    SDL_RenderCopy(renderer, selectionTexture, nullptr, &destRect);
    SDL_SetRenderTarget(renderer, nullptr);

    // Set selection position to top-left
    selectionRect.x = 0;
    selectionRect.y = 0;
    selectionOffset = ImVec2(0, 0);
    hasSelection = true;
}

void deleteSelection() {
    if (!hasSelection || activeLayerIndex < 0) return;

    // Clear selected area
    SDL_SetRenderTarget(renderer, layers[activeLayerIndex].texture);
    SDL_Rect clearRect = {
        selectionRect.x + (int)selectionOffset.x,
        selectionRect.y + (int)selectionOffset.y,
        selectionRect.w,
        selectionRect.h
    };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderFillRect(renderer, &clearRect);
    SDL_SetRenderTarget(renderer, nullptr);

    clearSelection();
    saveUndoState();
}

void mergeLayers() {
    if (layers.size() < 2) return;

    // Create new merged layer
    addLayer("Merged");

    // Render all visible layers to the new layer
    SDL_SetRenderTarget(renderer, layers[activeLayerIndex].texture);
    for (auto& layer : layers) {
        if (layer.visible && layer.texture != layers[activeLayerIndex].texture) {
            SDL_SetTextureAlphaMod(layer.texture, (Uint8)(layer.opacity * 255));
            SDL_RenderCopy(renderer, layer.texture, nullptr, nullptr);
        }
    }
    SDL_SetRenderTarget(renderer, nullptr);

    // Remove other layers
    for (int i = layers.size() - 2; i >= 0; i--) {
        if (layers[i].texture) SDL_DestroyTexture(layers[i].texture);
        layers.erase(layers.begin() + i);
    }
    activeLayerIndex = 0;
    saveUndoState();
}

void drawLine(SDL_Renderer* rend, ImVec2 start, ImVec2 end, const ImVec4& color, int thickness) {
    // Draw thick line by drawing multiple lines
    SDL_SetRenderDrawColor(rend,
        (Uint8)(color.x * 255),
        (Uint8)(color.y * 255),
        (Uint8)(color.z * 255),
        (Uint8)(color.w * 255));

    // Simple implementation for thickness
    if (thickness == 1) {
        SDL_RenderDrawLine(rend, (int)start.x, (int)start.y, (int)end.x, (int)end.y);
    } else {
        // Calculate perpendicular vector
        float dx = end.x - start.x;
        float dy = end.y - start.y;
        float length = sqrt(dx*dx + dy*dy);
        if (length < 0.001) return;

        dx /= length;
        dy /= length;

        // Draw multiple parallel lines
        float perpX = -dy * (thickness / 2.0f);
        float perpY = dx * (thickness / 2.0f);

        for (int i = 0; i < thickness; i++) {
            float offsetX = perpX * (i - thickness/2.0f);
            float offsetY = perpY * (i - thickness/2.0f);
            SDL_RenderDrawLine(rend,
                (int)(start.x + offsetX), (int)(start.y + offsetY),
                (int)(end.x + offsetX), (int)(end.y + offsetY));
        }
    }
}

void drawEraser(SDL_Renderer* rend, ImVec2 position, int size) {
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 0); // Transparent

    // Draw a circle for eraser
    int centerX = (int)position.x;
    int centerY = (int)position.y;
    int radius = size / 2;

    for (int w = -radius; w < radius; w++) {
        for (int h = -radius; h < radius; h++) {
            if (w*w + h*h <= radius*radius) {
                SDL_RenderDrawPoint(rend, centerX + w, centerY + h);
            }
        }
    }
}

void drawRectangle(SDL_Renderer* rend, ImVec2 start, ImVec2 end, const ImVec4& color, bool filled) {
    SDL_SetRenderDrawColor(rend,
        (Uint8)(color.x * 255),
        (Uint8)(color.y * 255),
        (Uint8)(color.z * 255),
        (Uint8)(color.w * 255));

    SDL_Rect rect = {
        (int)std::min(start.x, end.x),
        (int)std::min(start.y, end.y),
        (int)std::abs(end.x - start.x),
        (int)std::abs(end.y - start.y)
    };

    if (filled) {
        SDL_RenderFillRect(rend, &rect);
    } else {
        SDL_RenderDrawRect(rend, &rect);
    }
}

void drawCircle(SDL_Renderer* rend, ImVec2 center, int radius, const ImVec4& color, bool filled) {
    SDL_SetRenderDrawColor(rend,
        (Uint8)(color.x * 255),
        (Uint8)(color.y * 255),
        (Uint8)(color.z * 255),
        (Uint8)(color.w * 255));

    if (filled) {
        // Filled circle
        for (int w = -radius; w < radius; w++) {
            for (int h = -radius; h < radius; h++) {
                if (w*w + h*h <= radius*radius) {
                    SDL_RenderDrawPoint(rend, (int)center.x + w, (int)center.y + h);
                }
            }
        }
    } else {
        // Outline circle
        int x = radius;
        int y = 0;
        int err = 0;

        while (x >= y) {
            SDL_RenderDrawPoint(rend, (int)center.x + x, (int)center.y + y);
            SDL_RenderDrawPoint(rend, (int)center.x + y, (int)center.y + x);
            SDL_RenderDrawPoint(rend, (int)center.x - y, (int)center.y + x);
            SDL_RenderDrawPoint(rend, (int)center.x - x, (int)center.y + y);
            SDL_RenderDrawPoint(rend, (int)center.x - x, (int)center.y - y);
            SDL_RenderDrawPoint(rend, (int)center.x - y, (int)center.y - x);
            SDL_RenderDrawPoint(rend, (int)center.x + y, (int)center.y - x);
            SDL_RenderDrawPoint(rend, (int)center.x + x, (int)center.y - y);

            if (err <= 0) {
                y += 1;
                err += 2*y + 1;
            }
            if (err > 0) {
                x -= 1;
                err -= 2*x + 1;
            }
        }
    }
}

void drawTriangle(SDL_Renderer* rend, ImVec2 p1, ImVec2 p2, ImVec2 p3, const ImVec4& color) {
    SDL_SetRenderDrawColor(rend,
        (Uint8)(color.x * 255),
        (Uint8)(color.y * 255),
        (Uint8)(color.z * 255),
        (Uint8)(color.w * 255));

    SDL_RenderDrawLine(rend, (int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y);
    SDL_RenderDrawLine(rend, (int)p2.x, (int)p2.y, (int)p3.x, (int)p3.y);
    SDL_RenderDrawLine(rend, (int)p3.x, (int)p3.y, (int)p1.x, (int)p1.y);
}

void drawText(SDL_Renderer* rend, int x, int y, const std::string& text, const ImVec4& color) {
    if (text.empty()) return;

    TTF_Font* font = getFont(fontSize, textBold, textItalic);
    if (!font) {
        std::cerr << "Error: Could not load font: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_Color sdlColor = {
        (Uint8)(color.x * 255),
        (Uint8)(color.y * 255),
        (Uint8)(color.z * 255),
        255
    };

    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), sdlColor);
    if (!textSurface) {
        std::cerr << "Error: Could not render text: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(rend, textSurface);
    if (!textTexture) {
        std::cerr << "Error: Could not create texture from surface: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(textSurface);
        return;
    }

    SDL_Rect destRect = {x, y, textSurface->w, textSurface->h};
    SDL_RenderCopy(rend, textTexture, nullptr, &destRect);

    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

void drawGradient(SDL_Renderer* rend, ImVec2 start, ImVec2 end, const ImVec4& color1, const ImVec4& color2, GradientType type) {
    int steps = 100;
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float distance = sqrt(dx*dx + dy*dy);

    if (distance < 1.0f) return;

    dx /= distance;
    dy /= distance;

    for (int i = 0; i < steps; i++) {
        float t = i / (float)(steps - 1);
        ImVec4 color;
        color.x = color1.x + t * (color2.x - color1.x);
        color.y = color1.y + t * (color2.y - color1.y);
        color.z = color1.z + t * (color2.z - color1.z);
        color.w = color1.w + t * (color2.w - color1.w);

        SDL_SetRenderDrawColor(rend,
            (Uint8)(color.x * 255),
            (Uint8)(color.y * 255),
            (Uint8)(color.z * 255),
            (Uint8)(color.w * 255));

        if (type == LINEAR) {
            ImVec2 pos = {
                start.x + t * dx * distance,
                start.y + t * dy * distance
            };
            SDL_RenderDrawPoint(rend, (int)pos.x, (int)pos.y);
        }
        else if (type == RADIAL) {
            float radius = t * distance;
            int centerX = (int)start.x;
            int centerY = (int)start.y;

            for (int a = 0; a < 360; a += 5) {
                float rad = a * M_PI / 180.0f;
                int px = centerX + (int)(radius * cos(rad));
                int py = centerY + (int)(radius * sin(rad));
                SDL_RenderDrawPoint(rend, px, py);
            }
        }
        else if (type == ANGULAR) {
            float angle = t * 2 * M_PI;
            int px = start.x + (int)(distance * cos(angle));
            int py = start.y + (int)(distance * sin(angle));
            SDL_RenderDrawLine(rend, (int)start.x, (int)start.y, px, py);
        }
    }
}

void floodFill(int x, int y, ImVec4 fillColor) {
    // Bounds check
    if (x < 0 || x >= CANVAS_WIDTH || y < 0 || y >= CANVAS_HEIGHT) return;

    // Check if we have valid layers and active layer index
    if (layers.empty() || activeLayerIndex < 0 || activeLayerIndex >= layers.size()) {
        std::cerr << "Error: Invalid layer state in floodFill\n";

        return;
    }

    // Check if active layer texture exists
    if (!layers[activeLayerIndex].texture) {
        std::cerr << "Error: Active layer texture is null\n";
        return;
    }

    // Get target color from the active layer (not canvasBuffer)
    Uint32 targetColor;
    SDL_SetRenderTarget(renderer, layers[activeLayerIndex].texture);
    SDL_Rect pixel = {x, y, 1, 1};

    // Check if RenderReadPixels succeeds
    if (SDL_RenderReadPixels(renderer, &pixel, SDL_PIXELFORMAT_RGBA8888, &targetColor, sizeof(Uint32)) != 0) {
        std::cerr << "Error reading pixel: " << SDL_GetError() << "\n";
        SDL_SetRenderTarget(renderer, nullptr);
        return;
    }

    // Prepare fill color
    SDL_PixelFormat* format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
    if (!format) {
        std::cerr << "Error allocating pixel format: " << SDL_GetError() << "\n";
        SDL_SetRenderTarget(renderer, nullptr);
        return;
    }

    Uint32 fillRGBA = SDL_MapRGBA(format,
        (Uint8)(fillColor.x * 255),
        (Uint8)(fillColor.y * 255),
        (Uint8)(fillColor.z * 255),
        255);
    SDL_FreeFormat(format);

    // Don't fill if same color
    if (targetColor == fillRGBA) {
        SDL_SetRenderTarget(renderer, nullptr);
        return;
    }

    // Use scanline algorithm with stack limit to prevent stack overflow
    std::queue<std::pair<int, int>> pixels;
    pixels.push(std::make_pair(x, y));

    // Use a more memory-efficient visited array
    std::vector<std::vector<bool>> visited(CANVAS_WIDTH, std::vector<bool>(CANVAS_HEIGHT, false));

    // Set up rendering
    SDL_SetRenderDrawColor(renderer,
        (Uint8)(fillColor.x * 255),
        (Uint8)(fillColor.y * 255),
        (Uint8)(fillColor.z * 255),
        255);

    // Limit iterations to prevent infinite loops
    int maxIterations = CANVAS_WIDTH * CANVAS_HEIGHT * 300;
    int iterations = 0;

    while (!pixels.empty() && iterations < maxIterations) {
        iterations++;

        std::pair<int, int> p = pixels.front();
        pixels.pop();
        int px = p.first;
        int py = p.second;

        // Skip if out of bounds or already visited
        if (px < 0 || px >= CANVAS_WIDTH || py < 0 || py >= CANVAS_HEIGHT || visited[px][py])
            continue;

        // Check color match - read from the same layer we're filling
        Uint32 currentColor;
        pixel = {px, py, 1, 1};

        if (SDL_RenderReadPixels(renderer, &pixel, SDL_PIXELFORMAT_RGBA8888, &currentColor, sizeof(Uint32)) != 0) {
            std::cerr << "Error reading pixel during fill: " << SDL_GetError() << std::endl;
            break;  // Exit on error instead of continuing
        }

        if (currentColor != targetColor)
            continue;

        // Fill the pixel
        SDL_RenderDrawPoint(renderer, px, py);
        visited[px][py] = true;

        // Add neighbors (with bounds checking)
        if (px + 1 < CANVAS_WIDTH) pixels.push(std::make_pair(px + 1, py));
        if (px - 1 >= 0) pixels.push(std::make_pair(px - 1, py));
        if (py + 1 < CANVAS_HEIGHT) pixels.push(std::make_pair(px, py + 1));
        if (py - 1 >= 0) pixels.push(std::make_pair(px, py - 1));
    }

    if (iterations >= maxIterations) {
        std::cout << "Warning: Flood fill reached iteration limit\n";
    }

    SDL_SetRenderTarget(renderer, nullptr);
}

void handleKeyboard(SDL_Keycode key) {
    SDL_Keymod mod = SDL_GetModState();

    switch (key) {
        case SDLK_e:
            currentTool = ERASER;
            break;
        case SDLK_p:
            currentTool = PENCIL;
            break;
        case SDLK_r:
            currentTool = RECT;
            break;
        case SDLK_l:
            currentTool = LINE;
            break;
        case SDLK_f:
            currentTool = FILL;
            break;
        case SDLK_c:
            currentTool = CIRCLE;
            break;
        case SDLK_t:
            currentTool = TRIANGLE;
            break;
        case SDLK_g:
            currentTool = GRADIENT;
            break;
        case SDLK_s:
            currentTool = SELECT;
            break;
        case SDLK_x:
            // Swap primary and secondary colors
            std::swap(currColor, secondaryColor);
            break;
        case SDLK_v:
        // Vert. French for green.
            currColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
            break;
        case SDLK_b:
            currColor = ImVec4(0.0f, 0.0f, 1.0f, 1.0f); // Blue
            break;
        case SDLK_ESCAPE:
            currentTool = NONE;
            if (hasSelection) clearSelection();
            break;
        case SDLK_PLUS:
        case SDLK_EQUALS:
            if (mod & KMOD_CTRL) {
                brushSize = std::min(50, brushSize + 1);
            } else if (activeLayerIndex < layers.size() - 1) {
                activeLayerIndex++;
            }
            break;
        case SDLK_MINUS:
            if (mod & KMOD_CTRL) {
                brushSize = std::max(1, brushSize - 1);
            } else if (activeLayerIndex > 0) {
                activeLayerIndex--;
            }
            break;
        case SDLK_z:
            if (mod & KMOD_CTRL) {
                if (mod & KMOD_SHIFT) {
                    applyRedo();
                } else {
                    applyUndo();
                }
            }
            break;
        case SDLK_y:
            if (mod & KMOD_CTRL) {
                applyRedo();
            }
            break;
        case SDLK_m:
            if (mod & KMOD_CTRL) {
                mergeLayers();
            }
            break;
        case SDLK_DELETE:
            if (hasSelection) {
                deleteSelection();
            }
            break;
    }
}

void handleMouseClick(int button, int state, int x, int y) {
    // Adjust for canvas position
    x -= 10;
    y -= 10;

    if (x < 0 || y < 0 || x >= CANVAS_WIDTH || y >= CANVAS_HEIGHT) return;

    if (button == SDL_BUTTON_LEFT) {
        if (state == SDL_PRESSED) {
            isDrawing = true;
            startPos = ImVec2((float)x, (float)y);
            currentPos = startPos;

            // Tool-specific press actions
            switch (currentTool) {
                case FILL:
                    saveUndoState();
                    floodFill(x, y, currColor);
                    break;

                case TEXT:
                    saveUndoState();
                    SDL_SetRenderTarget(renderer, layers[activeLayerIndex].texture);
                    drawText(renderer, x, y, textContent, currColor);
                    SDL_SetRenderTarget(renderer, nullptr);
                    break;

                case SELECT:
                    // Start new selection
                    clearSelection();
                    hasSelection = true;
                    selectionRect.x = x;
                    selectionRect.y = y;
                    selectionRect.w = 0;
                    selectionRect.h = 0;
                    break;

                case GRADIENT:
                case PENCIL:
                case ERASER:
                    saveUndoState();
                    break;

                default:
                    break;
            }
        } else if (state == SDL_RELEASED) {
            isDrawing = false;

            // Finalize drawing on active layer
            if (currentTool != NONE && currentTool != FILL && currentTool != TEXT) {
                SDL_SetRenderTarget(renderer, layers[activeLayerIndex].texture);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

                switch (currentTool) {
                    case LINE:
                        drawLine(renderer, startPos, currentPos, currColor, brushSize);
                        break;

                    case RECT:
                        drawRectangle(renderer, startPos, currentPos, currColor, false);
                        break;

                    case CIRCLE: {
                        int radius = static_cast<int>(sqrt(pow(currentPos.x - startPos.x, 2) +
                                                    pow(currentPos.y - startPos.y, 2)));
                        drawCircle(renderer, startPos, radius, currColor, false);
                        break;
                    }

                    case TRIANGLE: {
                        ImVec2 p3 = ImVec2(startPos.x, currentPos.y);
                        drawTriangle(renderer, startPos, currentPos, p3, currColor);
                        break;
                    }

                    case GRADIENT:
                        drawGradient(renderer, startPos, currentPos, currColor, secondaryColor, gradientType);
                        break;

                    case SELECT:
                        // Finalize selection rectangle
                        selectionRect.w = (int)std::abs(currentPos.x - startPos.x);
                        selectionRect.h = (int)std::abs(currentPos.y - startPos.y);
                        selectionRect.x = (int)std::min(startPos.x, currentPos.x);
                        selectionRect.y = (int)std::min(startPos.y, currentPos.y);
                        copySelection();
                        selectionOffset = ImVec2(0, 0);
                        break;

                    case PENCIL:
                    case ERASER:
                        // Points are already drawn during motion
                        break;

                    default:
                        break;
                }

                SDL_SetRenderTarget(renderer, nullptr);
            }
        }
    }
    else if (button == SDL_BUTTON_RIGHT && state == SDL_PRESSED) {
        // Secondary color picker
        SDL_SetRenderTarget(renderer, canvasBuffer);
        SDL_Rect pixel = {x, y, 1, 1};
        Uint8 r, g, b, a;
        SDL_RenderReadPixels(renderer, &pixel, SDL_PIXELFORMAT_RGBA8888, &r, sizeof(Uint32));
        secondaryColor = ImVec4(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
        SDL_SetRenderTarget(renderer, nullptr);
    }
}

void handleMouseMotion(int x, int y) {
    // Adjust for canvas position
    x -= 10;
    y -= 10;

    if (x < 0 || y < 0 || x >= CANVAS_WIDTH || y >= CANVAS_HEIGHT) return;

    currentPos = ImVec2((float)x, (float)y);

    if (isDrawing) {
        // Draw directly to layer for some tools
        if (currentTool == PENCIL || currentTool == ERASER) {
            SDL_SetRenderTarget(renderer, layers[activeLayerIndex].texture);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

            if (currentTool == PENCIL) {
                SDL_SetRenderDrawColor(renderer,
                    (Uint8)(currColor.x * 255),
                    (Uint8)(currColor.y * 255),
                    (Uint8)(currColor.z * 255),
                    255);
                SDL_RenderDrawLine(renderer, (int)startPos.x, (int)startPos.y,
                                  (int)currentPos.x, (int)currentPos.y);
            } else if (currentTool == ERASER) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0); // Transparent
                drawEraser(renderer, currentPos, eraserSize);
            }

            SDL_SetRenderTarget(renderer, nullptr);
            startPos = currentPos;
        }
        else if (currentTool == SELECT && hasSelection && selectionTexture) {
            // Move selection
            selectionOffset.x += currentPos.x - startPos.x;
            selectionOffset.y += currentPos.y - startPos.y;
            startPos = currentPos;
        }
    }
}
