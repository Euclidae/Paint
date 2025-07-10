#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>
#include <map>
#include <memory>

// Forward declarations
class Layer;
struct TextState;
class Tool;
class Editor;

enum class AdjustmentType {
    NONE,
    BRIGHTNESS,
    CONTRAST,
    HUE_SATURATION,
    GAMMA
};

class Canvas {
public:
    static Canvas& getInstance();
    void init(SDL_Renderer* renderer);
    void cleanup();
    void render();
    
    // Canvas management
    void setupNewCanvas(int width, int height);
    void resizeCanvas(int newWidth, int newHeight);
    bool handleResizeEvent(const SDL_Event& event, const SDL_Point& mousePos);
    void applyInteractiveResize();
    void drawResizeHandles(SDL_Renderer* renderer);
    
    // Layer management
    void addLayer(const std::string& name = "New Layer", bool isTextLayer = false);
    void duplicateLayer(int index);
    void removeLayer(int index);
    void moveLayer(int fromIndex, int toIndex);
    void renameLayer(int index, const std::string& newName);
    void createLayerTexture(Layer& layer);
    
    // File operations
    void importImage(const char* filePath);
    void exportImage(const char* filePath, const char* format);
    
    // Image manipulations
    void cropImage();
    void rotateImage(int angle);
    void flipHorizontal(bool wholeCanvas = false);
    void flipVertical(bool wholeCanvas = false);
    void applyGrayscale();
    void applyBlur(int strength);
    void applySharpen(int strength = 2);
    void adjustContrast(float contrast);
    void applyFilter(int filterType);
    void applyEdgeDetection(); // Inspired by Snapchat-style effect
    void applyDirectionalBlur(int angle, int distance); // Motion blur in specific direction
    void applyShadowsHighlights(float shadows, float highlights); // Separate shadow/highlight control
    void applyColorBalance(float r, float g, float b); // RGB channel balance
    void applyCurves(float input, float output); // Simple curve adjustment
    void applyVibrance(float vibrance); // Smart saturation enhancement
    void addAdjustmentLayer(AdjustmentType type);
    void applyAdjustment(AdjustmentType type, float amount);
    void applyGradientMap(SDL_Color startColor, SDL_Color endColor);
    void addMaskToLayer(int layerIndex);
    
    // Smart object selection and transform
    int findLayerAtPoint(int x, int y); // Returns layer index with content at point
    void selectLayerAtPoint(int x, int y); // Auto-select layer with content
    void showTransformBox(int layerIndex); // Show transform handles around layer
    void hideTransformBox(); // Hide transform handles
    void handleTransformDrag(const SDL_Event& event, const SDL_Point& mousePos);
    void drawTransformBox(SDL_Renderer* renderer); // Draw the transform handles
    void applyTransform(); // Apply current transform to selected layer
    void deselectAll(); // Clear all selections and hide transform boxes
    
    // Font handling
    void clearFontCache();
    TTF_Font* getFont(int size, bool bold, bool italic);
    
    // Getters and setters
    SDL_Renderer* getRenderer() const { return m_renderer; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    Layer* getActiveLayer();
    int getActiveLayerIndex() const { return m_activeLayerIndex; }
    void setActiveLayerIndex(int index) { m_activeLayerIndex = index; }
    const std::vector<std::unique_ptr<Layer>>& getLayers() const { return m_layers; }
    SDL_Texture* getCanvasBuffer() const { return m_canvasBuffer; }
    
    // Selection management
    SDL_Rect getSelectionRect() const { return m_selectionRect; }
    void setSelectionRect(SDL_Rect rect) { m_selectionRect = rect; }
    bool hasSelection() const { return m_hasSelection; }
    void setHasSelection(bool has) { m_hasSelection = has; }
    SDL_Texture* getSelectionTexture() const { return m_selectionTexture; }
    void setSelectionTexture(SDL_Texture* texture) { m_selectionTexture = texture; }
    
    // Transform box state
    bool isTransformBoxVisible() const { return m_transformBoxVisible; }
    int getTransformLayer() const { return m_transformLayerIndex; }
    SDL_Rect getTransformRect() const { return m_transformRect; }
    
private:
    Canvas(); // Private constructor for singleton
    ~Canvas() = default;
    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;
    
    // Core properties
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_canvasBuffer = nullptr;
    int m_width = 1280;
    int m_height = 720;
    
    // Layers
    std::vector<std::unique_ptr<Layer>> m_layers;
    int m_activeLayerIndex = 0;
    
    // Font cache
    std::map<int, TTF_Font*> m_fontCache;
    
    // Filter tracking and buffer system to prevent crashes when combining filters
    enum class FilterType { NONE = -1, GRAYSCALE = 0, BLUR = 1, EDGE_DETECT = 2 };
    FilterType m_lastAppliedFilter = FilterType::NONE;
    bool m_filterInProgress = false;
    
    // HACK: Buffer image system to prevent filter stacking segfaults
    // Create hidden buffer, apply filter, then replace - genius workaround!
    SDL_Texture* m_filterBuffer = nullptr;
    void createFilterBuffer();
    void applyFilterBuffer();
    void cleanupFilterBuffer();
    
    // Selection state
    SDL_Rect m_selectionRect = {0, 0, 0, 0};
    bool m_hasSelection = false;
    SDL_Texture* m_selectionTexture = nullptr;
    
    // Transform box state for smart object selection
    bool m_transformBoxVisible = false;
    int m_transformLayerIndex = -1;
    SDL_Rect m_transformRect = {0, 0, 0, 0};
    SDL_Point m_transformStartMouse = {0, 0};
    SDL_Rect m_transformStartRect = {0, 0, 0, 0};
    int m_transformHandle = -1; // Which handle is being dragged
    bool m_isDraggingTransform = false;
    
    // Resize state
    SDL_Point m_resizeStartMouse = {0, 0};
    SDL_Rect m_resizeStartCanvas = {0, 0, 0, 0};
    int m_resizeCorner = -1;
    static constexpr int HANDLE_SIZE = 8;
    
    SDL_Surface* resizeImage(SDL_Surface* src, int newWidth, int newHeight);
    
    // Helper functions for layer flipping
    void flipLayerHorizontal(Layer* layer);
    void flipLayerVertical(Layer* layer);
    
    // Helper functions for smart object selection
    bool hasContentAtPoint(SDL_Texture* texture, int x, int y);
    SDL_Rect calculateLayerBounds(Layer* layer);
    int getTransformHandleAtPoint(int x, int y); // Returns handle index or -1
    void updateTransformRect(); // Update transform box based on layer content
};

// Helper function to get a reference to the canvas instance
inline Canvas& GetCanvas() {
    return Canvas::getInstance();
}