#pragma once
#include <SDL2/SDL.h>
#include "imgui/imgui.h"
#include <string>
#include <vector>
#include <memory>

class Canvas;

class Tool {
public:
    Tool() = default;
    virtual ~Tool() = default;
    
    Tool(const Tool&) = delete;
    Tool& operator=(const Tool&) = delete;
    
    virtual void handleMouseDown(const SDL_Event& event) = 0;
    virtual void handleMouseMove(const SDL_Event& event) = 0;
    virtual void handleMouseUp(const SDL_Event& event) = 0;
    virtual void render(SDL_Renderer* /* renderer */) {}
    virtual void cancel() {}
    
    virtual void setColor(const ImVec4& color) { m_color = color; }
    ImVec4 getColor() const { return m_color; }
    
    virtual void setSize(int size) { m_size = size; }
    int getSize() const { return m_size; }
    
    virtual const char* getName() const = 0;
    virtual const char* getTooltip() const { return ""; }
    
    virtual bool isDrawing() const { return m_isDrawing; }
    
protected:
    ImVec4 m_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);  
    int m_size = 1;
    bool m_isDrawing = false;
    ImVec2 m_startPos = ImVec2(0, 0);   
    ImVec2 m_currentPos = ImVec2(0, 0); 
    
    // Helper to convert ImVec to SDL as ImVec does it like OpenGL where stuff ranges in the 1s.
    // So if you multiply anything from that by 255, you get the color you want.
    // 1 = 255
    // x | x < 1 = x
    // Lost the plot slightly but you get the idea. Cross multiply that and you will have a means for conversion.
    SDL_Color toSDLColor(const ImVec4& color) const {
        return {
            static_cast<Uint8>(color.x * 255),
            static_cast<Uint8>(color.y * 255),
            static_cast<Uint8>(color.z * 255),
            static_cast<Uint8>(color.w * 255)
        };
    }
};

// I tried a few different approaches here but settled on this inheritance model.
// It gives us flexibility to add new tools without modifying existing code.
// Each tool handles its own rendering and event logic which keeps the main loop clean.
class PencilTool : public Tool {
public:
    void handleMouseDown(const SDL_Event& event) override;
    void handleMouseMove(const SDL_Event& event) override;
    void handleMouseUp(const SDL_Event& event) override;
    void render(SDL_Renderer* renderer) override;
    const char* getName() const override { return "Pencil"; }
    const char* getTooltip() const override { return "Draw freehand lines"; }
    
    // Brush types - different drawing styles
    void setBrushType(int type) { m_brushType = type; }
    int getBrushType() const { return m_brushType; }
    
private:
    int m_brushType = 0;
};

class EraserTool : public Tool {
public:
    // Initialize with transparent color and larger size for better erasing experience
    EraserTool() { m_color = ImVec4(0, 0, 0, 0); m_size = 20; }  // transparent, bigger than pencil
    void handleMouseDown(const SDL_Event& event) override;
    void handleMouseMove(const SDL_Event& event) override;
    void handleMouseUp(const SDL_Event& event) override;
    const char* getName() const override { return "Eraser"; }
    const char* getTooltip() const override { return "Erase parts of the image"; }
};

class LineTool : public Tool {
public:
    void handleMouseDown(const SDL_Event& event) override;
    void handleMouseMove(const SDL_Event& event) override;
    void handleMouseUp(const SDL_Event& event) override;
    void render(SDL_Renderer* renderer) override;
    const char* getName() const override { return "Line"; }
    const char* getTooltip() const override { return "Draw straight lines"; }
    
    // I think the bounds are 1-10 are pretty reasonable so I will create a selection based on this.
    // I am not sure how this tool is supposed to work precisely (I have an idea) but I think I might hack it together.
    // Contributors will likely help out with refining this later.
    void setLineCount(int count) { m_lineCount = count; }
    int getLineCount() const { return m_lineCount; }
    
private:
    int m_lineCount = 1;  // Default to single line
};

class RectangleTool : public Tool {
public:
    void handleMouseDown(const SDL_Event& event) override;
    void handleMouseMove(const SDL_Event& event) override;
    void handleMouseUp(const SDL_Event& event) override;
    void render(SDL_Renderer* renderer) override;
    void setFilled(bool filled) { m_filled = filled; }
    const char* getName() const override { return "Rectangle"; }
    const char* getTooltip() const override { return "Draw rectangles"; }
    
private:
    bool m_filled = false;
};

class CircleTool : public Tool {
public:
    void handleMouseDown(const SDL_Event& event) override;
    void handleMouseMove(const SDL_Event& event) override;
    void handleMouseUp(const SDL_Event& event) override;
    void render(SDL_Renderer* renderer) override;
    void setFilled(bool filled) { m_filled = filled; }
    const char* getName() const override { return "Circle"; }
    const char* getTooltip() const override { return "Draw circles"; }
    
private:
    bool m_filled = false;
};

class TriangleTool : public Tool {
public:
    void handleMouseDown(const SDL_Event& event) override;
    void handleMouseMove(const SDL_Event& event) override;
    void handleMouseUp(const SDL_Event& event) override;
    void render(SDL_Renderer* renderer) override;
    const char* getName() const override { return "Triangle"; }
    const char* getTooltip() const override { return "Draw triangles"; }
};
// AKA bucket.
class FillTool : public Tool {
public:
    void handleMouseDown(const SDL_Event& event) override;
    void handleMouseMove(const SDL_Event& /* event */) override { /* No operation needed */ }
    void handleMouseUp(const SDL_Event& /* event */) override { /* No operation needed */ }
    const char* getName() const override { return "Fill"; }
    const char* getTooltip() const override { return "Fill areas with color"; }
    
private:
    void floodFill(int x, int y, ImVec4 fillColor, ImVec4 targetColor = ImVec4(0,0,0,0));
    SDL_Texture* newTexture = nullptr; // See line 950 in the Toomanagers
};

class TextTool : public Tool {
public:
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
        std::string fontName = "Default";
        
        bool operator==(const TextBox& other) const {
            return content == other.content && 
                   rect.x == other.rect.x && 
                   rect.y == other.rect.y &&
                   fontSize == other.fontSize;
        }
        
        bool isEmpty() const {
            return content.empty() || content == "Sample Text";
        }
        
        std::string getEffectiveFontPath() const {
            if (!fontPath.empty()) return fontPath;
            return "arial.ttf";
        }
    };
    
    TextTool();
    ~TextTool();
    
    void handleMouseDown(const SDL_Event& event) override;
    void handleMouseMove(const SDL_Event& event) override;
    void handleMouseUp(const SDL_Event& event) override;
    void render(SDL_Renderer* renderer) override;
    const char* getName() const override { return "Text"; }
    const char* getTooltip() const override { return "Add text to the image"; }
    
    void createTextBox(int x, int y, int w, int h);
    void activateTextBox(int index);
    void deactivateAllTextBoxes();
    void finalizeTextBox(int index);
    void deleteTextBox(int index);
    void renderTextBoxToLayer(const TextBox& textBox);
    void renderTextBoxes(SDL_Renderer* renderer);
    
    void loadAvailableFonts();
    const std::vector<std::string>& getAvailableFonts() const { return m_availableFonts; }
    void setFontForTextBox(int index, const std::string& fontPath, const std::string& fontName);
    void addCustomFont(const std::string& fontPath, const std::string& fontName);
    
    const std::vector<TextBox>& getTextBoxes() const { return m_textBoxes; }
    TextBox* getTextBox(int index) { 
        if (index >= 0 && index < static_cast<int>(m_textBoxes.size())) {
            return &m_textBoxes[index];
        }
        return nullptr;
    }
    int getActiveTextBoxIndex() const { return m_activeTextBox; }
    size_t textBoxCount() const { return m_textBoxes.size(); }
    bool needsUpdate() const { return m_needsUpdate; }
    void setNeedsUpdate(bool update) { m_needsUpdate = update; }
    
private:
    std::vector<TextBox> m_textBoxes;
    int m_activeTextBox = -1;
    bool m_needsUpdate = false;
    std::vector<std::string> m_availableFonts;
    std::vector<std::string> m_fontNames;
    
    void renderTextBoxPreview(SDL_Renderer* renderer, const TextBox& textBox);
    void drawTextBoxBorder(SDL_Renderer* renderer, const TextBox& textBox, bool isActive);
    void scanFontDirectory(const std::string& directory);
};


class SelectionTool : public Tool {
public:
    void handleMouseDown(const SDL_Event& event) override;
    void handleMouseMove(const SDL_Event& event) override;
    void handleMouseUp(const SDL_Event& event) override;
    void render(SDL_Renderer* renderer) override;
    void cancel() override;
    const char* getName() const override { return "Selection"; }
    const char* getTooltip() const override { return "Select a region of the image"; }
};


class FloodSelectionTool : public Tool {
public:
    void handleMouseDown(const SDL_Event& event) override;
    void handleMouseMove(const SDL_Event& event) override;
    void handleMouseUp(const SDL_Event& event) override;
    void render(SDL_Renderer* renderer) override;
    void cancel() override;
    const char* getName() const override { return "Flood Select"; }
    const char* getTooltip() const override { return "Select similar colored areas"; }
    
    void setTolerance(int tolerance) { m_tolerance = tolerance; }
    int getTolerance() const { return m_tolerance; }
    
    void deleteSelectedPixels();
    
private:
    int m_tolerance = 10;
    std::vector<SDL_Point> m_selectedPixels;
    
    void floodSelect(int x, int y, ImVec4 targetColor);
    bool isColorSimilar(ImVec4 color1, ImVec4 color2) const;
    void clearSelection();
};

enum class GradientType { LINEAR, RADIAL, ANGULAR };

class GradientTool : public Tool {
public:
    GradientTool() : m_secondaryColor(1.0f, 1.0f, 1.0f, 1.0f), m_type(GradientType::LINEAR) {}
    
    void handleMouseDown(const SDL_Event& event) override;
    void handleMouseMove(const SDL_Event& event) override;
    void handleMouseUp(const SDL_Event& event) override;
    void render(SDL_Renderer* renderer) override;
    const char* getName() const override { return "Gradient"; }
    const char* getTooltip() const override { return "Create color gradients"; }
    
    void setSecondaryColor(const ImVec4& color) { m_secondaryColor = color; }
    ImVec4 getSecondaryColor() const { return m_secondaryColor; }
    
    void setGradientType(GradientType type) { m_type = type; }
    GradientType getGradientType() const { return m_type; }
    
private:
    ImVec4 m_secondaryColor;
    GradientType m_type;
    
    void drawGradient(SDL_Renderer* renderer, ImVec2 start, ImVec2 end, ImVec4 startColor, ImVec4 endColor);
};

class HealingTool : public Tool {
public:
    void handleMouseDown(const SDL_Event& event) override;
    void handleMouseMove(const SDL_Event& event) override;
    void handleMouseUp(const SDL_Event& event) override;
    const char* getName() const override { return "Healing"; }
    const char* getTooltip() const override { return "Healing brush for touch-ups"; }
    
private:
    void applyHealingAt(int x, int y);
};

class CloneStampTool : public Tool {
public:
    CloneStampTool();
    
    void handleMouseDown(const SDL_Event& event) override;
    void handleMouseMove(const SDL_Event& event) override;
    void handleMouseUp(const SDL_Event& event) override;
    void render(SDL_Renderer* renderer) override;
    const char* getName() const override { return "Clone Stamp"; }
    const char* getTooltip() const override { return "Alt+click to set source, then paint to clone"; }
    
    void setSourcePoint(int x, int y);
    bool hasSourcePoint() const { return m_hasSourcePoint; }
    
private:
    bool m_hasSourcePoint = false;
    SDL_Point m_sourcePoint = {0, 0};
    bool m_isCloning = false;
    
    void cloneAt(int x, int y);
    void drawSourcePreview(SDL_Renderer* renderer);
};


class Editor;

class ToolManager {
public:
    static ToolManager& getInstance();
    
    void init();
    void cleanup();
    
    void handleSDLEvent(const SDL_Event& event);
    void render(SDL_Renderer* renderer);
    
    // Tool selection
    void setCurrentTool(int toolIndex);
    Tool* getCurrentTool() const { return m_currentTool; }
    int getCurrentToolIndex() const { return m_currentToolIndex; }
    
    // Tool properties
    void setPrimaryColor(const ImVec4& color);
    ImVec4 getPrimaryColor() const;
    
    void setSecondaryColor(const ImVec4& color);
    ImVec4 getSecondaryColor() const;
    
    void setBrushSize(int size);
    int getBrushSize() const;
    
    void setEraserSize(int size);
    int getEraserSize() const;
    
    TextTool* getTextTool() const;
    
    GradientTool* getGradientTool() const;
    
private:
    ToolManager();
    ~ToolManager();
    ToolManager(const ToolManager&) = delete;
    ToolManager& operator=(const ToolManager&) = delete;
    
    std::vector<std::unique_ptr<Tool>> m_tools;
    Tool* m_currentTool = nullptr;
    int m_currentToolIndex = -1;
    
    ImVec4 m_primaryColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    ImVec4 m_secondaryColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    int m_brushSize = 5;
    int m_eraserSize = 20;
};


inline ToolManager& GetToolManager() {
    return ToolManager::getInstance();
}