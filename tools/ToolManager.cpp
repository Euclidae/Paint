#include "Tool.hpp"
#include "../canvas/Canvas.hpp"
#include "../canvas/Layer.hpp"
#include "../editor/Editor.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <queue>
#include <utility>
#include <set>

// TODO: Clean up this file, it's getting messy
// N.B: Some of these drawing functions could use optimization
// COMPLETE - Brush tool

// Singleton pattern - yeah I know globals are bad but this works
[[nodiscard("This is a singleton so it needs to be referenced.")]]ToolManager& ToolManager::getInstance() {
    static ToolManager instance;
    return instance;
}

ToolManager::ToolManager() : m_currentTool(nullptr), m_currentToolIndex(0) {
    // Initialize with default colors
    m_primaryColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);  // black
    m_secondaryColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // white
}

ToolManager::~ToolManager() {
    cleanup();
}

void ToolManager::init() {
    m_tools.push_back(std::make_unique<PencilTool>());
    m_tools.push_back(std::make_unique<EraserTool>());
    m_tools.push_back(std::make_unique<LineTool>());
    m_tools.push_back(std::make_unique<RectangleTool>());
    m_tools.push_back(std::make_unique<CircleTool>());
    m_tools.push_back(std::make_unique<TriangleTool>());
    m_tools.push_back(std::make_unique<FillTool>());
    m_tools.push_back(std::make_unique<SelectionTool>());
    m_tools.push_back(std::make_unique<FloodSelectionTool>());
    m_tools.push_back(std::make_unique<TextTool>());
    m_tools.push_back(std::make_unique<GradientTool>());
    m_tools.push_back(std::make_unique<HealingTool>());
    m_tools.push_back(std::make_unique<CloneStampTool>());

    // Set default tool
    setCurrentTool(0);
}

void ToolManager::cleanup() {
    m_currentTool = nullptr;
    m_tools.clear();
}

void ToolManager::handleSDLEvent(const SDL_Event& event) {
    if (!m_currentTool) return;

    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            m_currentTool->handleMouseDown(event);
            break;

        case SDL_MOUSEMOTION:
            m_currentTool->handleMouseMove(event);
            break;

        case SDL_MOUSEBUTTONUP:
            m_currentTool->handleMouseUp(event);
            break;

        case SDL_KEYDOWN:
            // Handle escape key to cancel current tool operation
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                m_currentTool->cancel();
            }
            // Handle Ctrl+D to deselect all selections
            else if (event.key.keysym.sym == SDLK_d && (SDL_GetModState() & KMOD_CTRL)) {
                GetCanvas().deselectAll();
            }
            // Handle Delete key for flood selection - delete selected pixels
            else if (event.key.keysym.sym == SDLK_DELETE && m_currentToolIndex == 8) {
                FloodSelectionTool* floodTool = static_cast<FloodSelectionTool*>(m_currentTool);
                floodTool->deleteSelectedPixels();
            }
            break;
    }
}

void ToolManager::render(SDL_Renderer* renderer) {
    if (m_currentTool && m_currentTool->isDrawing()) {
        m_currentTool->render(renderer);
    }
}

void ToolManager::setCurrentTool(int toolIndex) {
    if (toolIndex < 0 || toolIndex >= static_cast<int>(m_tools.size())) {
        return;  // bail out if invalid index
    }

    m_currentToolIndex = toolIndex;
    m_currentTool = m_tools[toolIndex].get();

    // Make sure tool has current color
    m_currentTool->setColor(m_primaryColor);

    // Set size based on tool type - eraser needs bigger default
    if (toolIndex == 1) { // Eraser tool
        m_currentTool->setSize(m_eraserSize);
    } else {
        m_currentTool->setSize(m_brushSize);
    }

    // Set gradient tool's secondary color if applicable
    GradientTool* gradientTool = getGradientTool();
    if (gradientTool) {
        gradientTool->setSecondaryColor(m_secondaryColor);
    }
}

void ToolManager::setPrimaryColor(const ImVec4& color) {
    m_primaryColor = color;
    if (m_currentTool) {
        m_currentTool->setColor(color);
    }
}

ImVec4 ToolManager::getPrimaryColor() const {
    return m_primaryColor;
}

void ToolManager::setSecondaryColor(const ImVec4& color) {
    m_secondaryColor = color;

    // Update gradient tool if it exists
    GradientTool* gradientTool = getGradientTool();
    if (gradientTool) {
        gradientTool->setSecondaryColor(color);
    }
}

ImVec4 ToolManager::getSecondaryColor() const {
    return m_secondaryColor;
}

void ToolManager::setBrushSize(int size) {
    m_brushSize = size;

    // Update current tool if it's not an eraser
    if (m_currentTool && !dynamic_cast<EraserTool*>(m_currentTool)) {
        m_currentTool->setSize(size);
    }
}

int ToolManager::getBrushSize() const {
    return m_brushSize;
}

void ToolManager::setEraserSize(int size) {
    m_eraserSize = size;

    // Update current tool if it's an eraser
    if (m_currentTool && dynamic_cast<EraserTool*>(m_currentTool)) {
        m_currentTool->setSize(size);
    }
}

int ToolManager::getEraserSize() const {
    return m_eraserSize;
}

TextTool* ToolManager::getTextTool() const {
    for (const auto& tool : m_tools) {
        TextTool* textTool = dynamic_cast<TextTool*>(tool.get());
        if (textTool) {
            return textTool;
        }
    }
    return nullptr;
}

GradientTool* ToolManager::getGradientTool() const {
    for (const auto& tool : m_tools) {
        GradientTool* gradientTool = dynamic_cast<GradientTool*>(tool.get());
        if (gradientTool) {
            return gradientTool;
        }
    }
    return nullptr;
}

// Basic implementations of tool methods

// PencilTool implementation
void PencilTool::handleMouseDown(const SDL_Event& event) {
    m_isDrawing = true;
    m_startPos = ImVec2(event.button.x, event.button.y);
    m_currentPos = m_startPos;

    // Draw a single point
    Canvas& canvas = Canvas::getInstance();
    Layer* activeLayer = canvas.getActiveLayer();

    if (activeLayer && !activeLayer->isLocked()) {
        SDL_Renderer* renderer = canvas.getRenderer();
        SDL_SetRenderTarget(renderer, activeLayer->getTexture());

        SDL_SetRenderDrawColor(renderer,
            static_cast<Uint8>(m_color.x * 255),
            static_cast<Uint8>(m_color.y * 255),
            static_cast<Uint8>(m_color.z * 255),
            static_cast<Uint8>(m_color.w * 255));

        // Draw a filled circle for the brush tip
        const int radius = m_size / 2;
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x*x + y*y <= radius*radius) {
                    SDL_RenderDrawPoint(renderer,
                        static_cast<int>(m_currentPos.x) + x,
                        static_cast<int>(m_currentPos.y) + y);
                }
            }
        }

        SDL_SetRenderTarget(renderer, nullptr);
    }

    // Save state for undo
    Editor::getInstance().saveUndoState();
}

void PencilTool::handleMouseMove(const SDL_Event& event) {
    if (!m_isDrawing) return;

    ImVec2 newPos(event.motion.x, event.motion.y);

    Canvas& canvas = Canvas::getInstance();
    Layer* activeLayer = canvas.getActiveLayer();

    if (activeLayer && !activeLayer->isLocked()) {
        SDL_Renderer* renderer = canvas.getRenderer();
        SDL_SetRenderTarget(renderer, activeLayer->getTexture());

        SDL_SetRenderDrawColor(renderer,
            static_cast<Uint8>(m_color.x * 255),
            static_cast<Uint8>(m_color.y * 255),
            static_cast<Uint8>(m_color.z * 255),
            static_cast<Uint8>(m_color.w * 255));

        // Draw line from previous position to current position
        const int radius = m_size / 2;
        const float dist = std::sqrt(
            (newPos.x - m_currentPos.x) * (newPos.x - m_currentPos.x) +
            (newPos.y - m_currentPos.y) * (newPos.y - m_currentPos.y)
        );

        // Different brush types give different drawing effects
        switch (m_brushType) {
            case 0: // Normal brush - solid circle
                if (dist < 1.0f) {
                    // Just draw a single point if we didn't move much
                    for (int y = -radius; y <= radius; y++) {
                        for (int x = -radius; x <= radius; x++) {
                            if (x*x + y*y <= radius*radius) {
                                SDL_RenderDrawPoint(renderer,
                                    static_cast<int>(newPos.x) + x,
                                    static_cast<int>(newPos.y) + y);
                            }
                        }
                    }
                } else {
                    // Draw a thick line by interpolating between points
                    const int steps = static_cast<int>(dist) + 1;
                    for (int i = 0; i <= steps; i++) {
                        float t = static_cast<float>(i) / steps;
                        float x = m_currentPos.x + t * (newPos.x - m_currentPos.x);
                        float y = m_currentPos.y + t * (newPos.y - m_currentPos.y);

                        for (int dy = -radius; dy <= radius; dy++) {
                            for (int dx = -radius; dx <= radius; dx++) {
                                if (dx*dx + dy*dy <= radius*radius) {
                                    SDL_RenderDrawPoint(renderer,
                                        static_cast<int>(x) + dx,
                                        static_cast<int>(y) + dy);
                                }
                            }
                        }
                    }
                }
                break;

            case 1: // Textured brush - random dot pattern
                {
                    const int steps = static_cast<int>(dist) + 1;
                    for (int i = 0; i <= steps; i++) {
                        float t = static_cast<float>(i) / steps;
                        float x = m_currentPos.x + t * (newPos.x - m_currentPos.x);
                        float y = m_currentPos.y + t * (newPos.y - m_currentPos.y);

                        // Random texture pattern - skip some pixels for texture effect
                        for (int dy = -radius; dy <= radius; dy++) {
                            for (int dx = -radius; dx <= radius; dx++) {
                                if (dx*dx + dy*dy <= radius*radius) {
                                    // Skip some pixels randomly for texture
                                    if (rand() % 3 == 0) {  // 1/3 chance to draw
                                        SDL_RenderDrawPoint(renderer,
                                            static_cast<int>(x) + dx,
                                            static_cast<int>(y) + dy);
                                    }
                                }
                            }
                        }
                    }
                }
                break;

            case 2: // Soft brush - gradient falloff
                {
                    const int steps = static_cast<int>(dist) + 1;
                    for (int i = 0; i <= steps; i++) {
                        float t = static_cast<float>(i) / steps;
                        float x = m_currentPos.x + t * (newPos.x - m_currentPos.x);
                        float y = m_currentPos.y + t * (newPos.y - m_currentPos.y);

                        // Soft brush with alpha falloff
                        for (int dy = -radius; dy <= radius; dy++) {
                            for (int dx = -radius; dx <= radius; dx++) {
                                float distance = std::sqrt(dx*dx + dy*dy);
                                if (distance <= radius) {
                                    // Calculate alpha based on distance from center
                                    float alpha = 1.0f - (distance / radius);

                                    // Set color with calculated alpha
                                    SDL_SetRenderDrawColor(renderer,
                                        static_cast<Uint8>(m_color.x * 255),
                                        static_cast<Uint8>(m_color.y * 255),
                                        static_cast<Uint8>(m_color.z * 255),
                                        static_cast<Uint8>(m_color.w * alpha * 255));

                                    SDL_RenderDrawPoint(renderer,
                                        static_cast<int>(x) + dx,
                                        static_cast<int>(y) + dy);
                                }
                            }
                        }
                    }
                }
                break;
        }

        /* Old single brush implementation - kept for reference
        if (dist < 1.0f) {
            // Just draw a single point if we didn't move much
            for (int y = -radius; y <= radius; y++) {
                for (int x = -radius; x <= radius; x++) {
                    if (x*x + y*y <= radius*radius) {
                        SDL_RenderDrawPoint(renderer,
                            static_cast<int>(newPos.x) + x,
                            static_cast<int>(newPos.y) + y);
                    }
                }
            }
        } else {
            // Draw a thick line by interpolating between points
            const int steps = static_cast<int>(dist) + 1;
            for (int i = 0; i <= steps; i++) {
                float t = static_cast<float>(i) / steps;
                float x = m_currentPos.x + t * (newPos.x - m_currentPos.x);
                float y = m_currentPos.y + t * (newPos.y - m_currentPos.y);

                for (int dy = -radius; dy <= radius; dy++) {
                    for (int dx = -radius; dx <= radius; dx++) {
                        if (dx*dx + dy*dy <= radius*radius) {
                            SDL_RenderDrawPoint(renderer,
                                static_cast<int>(x) + dx,
                                static_cast<int>(y) + dy);
                        }
                    }
                }
            }
        }
        */

        SDL_SetRenderTarget(renderer, nullptr);
    }

    m_currentPos = newPos;
}

void PencilTool::handleMouseUp(const SDL_Event&) {
    m_isDrawing = false;
}

void PencilTool::render(SDL_Renderer* renderer) {
    // Preview brush size when hovering - helps with different brush types
    if (!m_isDrawing) {
        // Show a preview circle of the brush size
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 128);
        const int radius = m_size / 2;

        // Get mouse position for preview
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        // Draw preview circle outline
        for (int angle = 0; angle < 360; angle += 5) {
            float rad = angle * M_PI / 180.0f;
            int x = mouseX + (int)(radius * cos(rad));
            int y = mouseY + (int)(radius * sin(rad));
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }
}

// EraserTool implementation
void EraserTool::handleMouseDown(const SDL_Event& event) {
    m_isDrawing = true;
    m_startPos = ImVec2(event.button.x, event.button.y);
    m_currentPos = m_startPos;

    Canvas& canvas = Canvas::getInstance();
    Layer* activeLayer = canvas.getActiveLayer();

    if (activeLayer && !activeLayer->isLocked()) {
        SDL_Renderer* renderer = canvas.getRenderer();
        SDL_SetRenderTarget(renderer, activeLayer->getTexture());

        // Set up blending for erasing (transparent)
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

        // Draw a filled circle for the eraser
        const int radius = m_size / 2;
        SDL_Rect rect = {
            static_cast<int>(m_currentPos.x) - radius,
            static_cast<int>(m_currentPos.y) - radius,
            m_size,
            m_size
        };
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderTarget(renderer, nullptr);
    }

    // Save state for undo
    Editor::getInstance().saveUndoState();
}

void EraserTool::handleMouseMove(const SDL_Event& event) {
    if (!m_isDrawing) return;

    ImVec2 newPos(event.motion.x, event.motion.y);

    Canvas& canvas = Canvas::getInstance();
    Layer* activeLayer = canvas.getActiveLayer();

    if (activeLayer && !activeLayer->isLocked()) {
        SDL_Renderer* renderer = canvas.getRenderer();
        SDL_SetRenderTarget(renderer, activeLayer->getTexture());

        // Set up blending for erasing (transparent)
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

        // Draw line from previous position to current position
        const int radius = m_size / 2;
        const float dist = std::sqrt(
            (newPos.x - m_currentPos.x) * (newPos.x - m_currentPos.x) +
            (newPos.y - m_currentPos.y) * (newPos.y - m_currentPos.y)
        );

        if (dist < 1.0f) {
            // Draw a single point if distance is very small
            SDL_Rect rect = {
                static_cast<int>(newPos.x) - radius,
                static_cast<int>(newPos.y) - radius,
                m_size,
                m_size
            };
            SDL_RenderFillRect(renderer, &rect);
        } else {
            // Interpolate points for a smooth line
            const float step = 1.0f / dist;
            for (float t = 0; t <= 1.0f; t += step) {
                float x = m_currentPos.x + (newPos.x - m_currentPos.x) * t;
                float y = m_currentPos.y + (newPos.y - m_currentPos.y) * t;

                SDL_Rect rect = {
                    static_cast<int>(x) - radius,
                    static_cast<int>(y) - radius,
                    m_size,
                    m_size
                };
                SDL_RenderFillRect(renderer, &rect);
            }
        }

        SDL_SetRenderTarget(renderer, nullptr);
    }

    m_currentPos = newPos;
}

void EraserTool::handleMouseUp(const SDL_Event&) {
    m_isDrawing = false;
}

// LineTool implementation
void LineTool::handleMouseDown(const SDL_Event& event) {
    m_isDrawing = true;
    m_startPos = ImVec2(event.button.x, event.button.y);
    m_currentPos = m_startPos;

    // Save state for undo
    Editor::getInstance().saveUndoState();
}

void LineTool::handleMouseMove(const SDL_Event& event) {
    if (!m_isDrawing) return;
    m_currentPos = ImVec2(event.motion.x, event.motion.y);
}

void LineTool::handleMouseUp(const SDL_Event&) {
    if (!m_isDrawing) return;

    Canvas& canvas = Canvas::getInstance();
    Layer* activeLayer = canvas.getActiveLayer();

    if (activeLayer && !activeLayer->isLocked()) {
        SDL_Renderer* renderer = canvas.getRenderer();
        SDL_SetRenderTarget(renderer, activeLayer->getTexture());

        SDL_SetRenderDrawColor(renderer,
            static_cast<Uint8>(m_color.x * 255),
            static_cast<Uint8>(m_color.y * 255),
            static_cast<Uint8>(m_color.z * 255),
            static_cast<Uint8>(m_color.w * 255));

        // Draw multiple lines with slight variations for artistic effect
        for (int lineNum = 0; lineNum < m_lineCount; lineNum++) {
            // Add some randomness to each line - not too much though
            float offsetX = (lineNum > 0) ? (rand() % 5 - 2) : 0;  // -2 to +2 pixels
            float offsetY = (lineNum > 0) ? (rand() % 5 - 2) : 0;

            ImVec2 start = ImVec2(m_startPos.x + offsetX, m_startPos.y + offsetY);
            ImVec2 end = ImVec2(m_currentPos.x + offsetX, m_currentPos.y + offsetY);

            const int radius = m_size / 2;
            const float dist = std::sqrt(
                (end.x - start.x) * (end.x - start.x) +
                (end.y - start.y) * (end.y - start.y)
            );

            if (dist < 1.0f) {
                // Draw a single point if distance is very small
                for (int y = -radius; y <= radius; y++) {
                    for (int x = -radius; x <= radius; x++) {
                        if (x*x + y*y <= radius*radius) {
                            SDL_RenderDrawPoint(renderer,
                                static_cast<int>(start.x) + x,
                                static_cast<int>(start.y) + y);
                        }
                    }
                }
            } else {
                // Draw a thick line by interpolation
                const float step = 1.0f / dist;
                for (float t = 0; t <= 1.0f; t += step) {
                    float x = start.x + (end.x - start.x) * t;
                    float y = start.y + (end.y - start.y) * t;

                    for (int cy = -radius; cy <= radius; cy++) {
                        for (int cx = -radius; cx <= radius; cx++) {
                            if (cx*cx + cy*cy <= radius*radius) {
                                SDL_RenderDrawPoint(renderer,
                                    static_cast<int>(x) + cx,
                                    static_cast<int>(y) + cy);
                            }
                        }
                    }
                }
            }
        }

        SDL_SetRenderTarget(renderer, nullptr);
    }

    m_isDrawing = false;
}

void LineTool::render(SDL_Renderer* renderer) {
    if (!m_isDrawing) return;

    // Draw preview lines - show what will be drawn
    SDL_SetRenderDrawColor(renderer,
        static_cast<Uint8>(m_color.x * 255),
        static_cast<Uint8>(m_color.y * 255),
        static_cast<Uint8>(m_color.z * 255),
        static_cast<Uint8>(m_color.w * 255));

    // Draw preview of multiple lines
    for (int lineNum = 0; lineNum < m_lineCount; lineNum++) {
        // Show slight offsets in preview but don't make it too distracting
        float offsetX = (lineNum > 0) ? (lineNum - 1) * 0.5f : 0;
        float offsetY = (lineNum > 0) ? (lineNum - 1) * 0.5f : 0;

        ImVec2 start = ImVec2(m_startPos.x + offsetX, m_startPos.y + offsetY);
        ImVec2 end = ImVec2(m_currentPos.x + offsetX, m_currentPos.y + offsetY);

        // Draw a thick line
        const int radius = m_size / 2;
        const float dist = std::sqrt(
            (end.x - start.x) * (end.x - start.x) +
            (end.y - start.y) * (end.y - start.y)
        );

    if (dist < 1.0f) {
        // Draw a single point if distance is very small
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x*x + y*y <= radius*radius) {
                    SDL_RenderDrawPoint(renderer,
                        static_cast<int>(start.x) + x,
                        static_cast<int>(start.y) + y);
                }
            }
        }
    } else {
        // Draw a thick line by interpolation
        const float step = 1.0f / dist;
        for (float t = 0; t <= 1.0f; t += step) {
            float x = start.x + (end.x - start.x) * t;
            float y = start.y + (end.y - start.y) * t;

            for (int cy = -radius; cy <= radius; cy++) {
                for (int cx = -radius; cx <= radius; cx++) {
                    if (cx*cx + cy*cy <= radius*radius) {
                        SDL_RenderDrawPoint(renderer,
                            static_cast<int>(x) + cx,
                            static_cast<int>(y) + cy);
                    }
                }
            }
        }
    }
}
}

// RectangleTool implementation
void RectangleTool::handleMouseDown(const SDL_Event& event) {
    m_isDrawing = true;
    m_startPos = ImVec2(event.button.x, event.button.y);
    m_currentPos = m_startPos;

    // Save state for undo
    Editor::getInstance().saveUndoState();
}

void RectangleTool::handleMouseMove(const SDL_Event& event) {
    if (!m_isDrawing) return;
    m_currentPos = ImVec2(event.motion.x, event.motion.y);
}

void RectangleTool::handleMouseUp(const SDL_Event&) {
    if (!m_isDrawing) return;

    Canvas& canvas = GetCanvas();
    Layer* activeLayer = canvas.getActiveLayer();

    if (activeLayer && !activeLayer->isLocked()) {
        SDL_Renderer* renderer = canvas.getRenderer();
        SDL_SetRenderTarget(renderer, activeLayer->getTexture());

        // Draw the final rectangle
        SDL_SetRenderDrawColor(renderer,
            static_cast<Uint8>(m_color.x * 255),
            static_cast<Uint8>(m_color.y * 255),
            static_cast<Uint8>(m_color.z * 255),
            static_cast<Uint8>(m_color.w * 255));

        // Calculate rectangle dimensions
        int x = std::min(static_cast<int>(m_startPos.x), static_cast<int>(m_currentPos.x));
        int y = std::min(static_cast<int>(m_startPos.y), static_cast<int>(m_currentPos.y));
        int w = std::abs(static_cast<int>(m_currentPos.x - m_startPos.x));
        int h = std::abs(static_cast<int>(m_currentPos.y - m_startPos.y));
        SDL_Rect rect = {x, y, w, h};

        if (m_filled) {
            SDL_RenderFillRect(renderer, &rect);
        } else {
            // Draw with the specified thickness
            for (int i = 0; i < m_size; i++) {
                SDL_Rect border = {x - i, y - i, w + i * 2, h + i * 2};
                SDL_RenderDrawRect(renderer, &border);
            }
        }

        SDL_SetRenderTarget(renderer, nullptr);
    }

    m_isDrawing = false;
}

void RectangleTool::render(SDL_Renderer* renderer) {
    if (!m_isDrawing) return;

    // Draw preview rectangle
    SDL_SetRenderDrawColor(renderer,
        static_cast<Uint8>(m_color.x * 255),
        static_cast<Uint8>(m_color.y * 255),
        static_cast<Uint8>(m_color.z * 255),
        static_cast<Uint8>(m_color.w * 255));

    // Calculate rectangle dimensions
    int x = std::min(static_cast<int>(m_startPos.x), static_cast<int>(m_currentPos.x));
    int y = std::min(static_cast<int>(m_startPos.y), static_cast<int>(m_currentPos.y));
    int w = std::abs(static_cast<int>(m_currentPos.x - m_startPos.x));
    int h = std::abs(static_cast<int>(m_currentPos.y - m_startPos.y));
    SDL_Rect rect = {x, y, w, h};

    if (m_filled) {
        SDL_RenderFillRect(renderer, &rect);
    } else {
        // Draw with the specified thickness
        for (int i = 0; i < m_size; i++) {
            SDL_Rect border = {x - i, y - i, w + i * 2, h + i * 2};
            SDL_RenderDrawRect(renderer, &border);
        }
    }
}

// CircleTool implementation
void CircleTool::handleMouseDown(const SDL_Event& event) {
    m_isDrawing = true;
    m_startPos = ImVec2(event.button.x, event.button.y);
    m_currentPos = m_startPos;

    // Save state for undo
    Editor::getInstance().saveUndoState();
}

void CircleTool::handleMouseMove(const SDL_Event& event) {
    if (!m_isDrawing) return;
    m_currentPos = ImVec2(event.motion.x, event.motion.y);
}

void CircleTool::handleMouseUp(const SDL_Event&) {
    if (!m_isDrawing) return;

    Canvas& canvas = GetCanvas();
    Layer* activeLayer = canvas.getActiveLayer();

    if (activeLayer && !activeLayer->isLocked()) {
        SDL_Renderer* renderer = canvas.getRenderer();
        SDL_SetRenderTarget(renderer, activeLayer->getTexture());

        // Calculate circle radius and center
        float dx = m_currentPos.x - m_startPos.x;
        float dy = m_currentPos.y - m_startPos.y;
        int radius = static_cast<int>(std::sqrt(dx*dx + dy*dy));
        int centerX = static_cast<int>(m_startPos.x);
        int centerY = static_cast<int>(m_startPos.y);

        // Set color
        SDL_SetRenderDrawColor(renderer,
            static_cast<Uint8>(m_color.x * 255),
            static_cast<Uint8>(m_color.y * 255),
            static_cast<Uint8>(m_color.z * 255),
            static_cast<Uint8>(m_color.w * 255));

        // Draw the circle
        if (m_filled) {
            // Fill the circle
            for (int y = -radius; y <= radius; y++) {
                for (int x = -radius; x <= radius; x++) {
                    if (x*x + y*y <= radius*radius) {
                        SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
                    }
                }
            }
        } else {
            // Draw the outline
            for (int thickness = 0; thickness < m_size; thickness++) {
                int r = radius - thickness;
                if (r < 0) break;

                for (int y = -r; y <= r; y++) {
                    for (int x = -r; x <= r; x++) {
                        int distSq = x*x + y*y;
                        if (distSq <= r*r && distSq >= (r-1)*(r-1)) {
                            SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
                        }
                    }
                }
            }
        }

        SDL_SetRenderTarget(renderer, nullptr);
    }

    m_isDrawing = false;
}

void CircleTool::render(SDL_Renderer* renderer) {
    if (!m_isDrawing) return;

    // Calculate circle radius and center
    float dx = m_currentPos.x - m_startPos.x;
    float dy = m_currentPos.y - m_startPos.y;
    int radius = static_cast<int>(std::sqrt(dx*dx + dy*dy));
    int centerX = static_cast<int>(m_startPos.x);
    int centerY = static_cast<int>(m_startPos.y);

    // Set color
    SDL_SetRenderDrawColor(renderer,
        static_cast<Uint8>(m_color.x * 255),
        static_cast<Uint8>(m_color.y * 255),
        static_cast<Uint8>(m_color.z * 255),
        static_cast<Uint8>(m_color.w * 255));

    // Draw the circle
    if (m_filled) {
        // Fill the circle
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x*x + y*y <= radius*radius) {
                    SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
                }
            }
        }
    } else {
        // Draw the outline
        for (int thickness = 0; thickness < m_size; thickness++) {
            int r = radius - thickness;
            if (r < 0) break;

            for (int y = -r; y <= r; y++) {
                for (int x = -r; x <= r; x++) {
                    int distSq = x*x + y*y;
                    if (distSq <= r*r && distSq >= (r-1)*(r-1)) {
                        SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
                    }
                }
            }
        }
    }
}

// TriangleTool implementation
void TriangleTool::handleMouseDown(const SDL_Event& event) {
    m_isDrawing = true;
    m_startPos = ImVec2(event.button.x, event.button.y);
    m_currentPos = m_startPos;

    // Save state for undo
    Editor::getInstance().saveUndoState();
}

void TriangleTool::handleMouseMove(const SDL_Event& event) {
    if (!m_isDrawing) return;
    m_currentPos = ImVec2(event.motion.x, event.motion.y);
}

void TriangleTool::handleMouseUp(const SDL_Event&) {
    if (!m_isDrawing) return;

    Canvas& canvas = GetCanvas();
    Layer* activeLayer = canvas.getActiveLayer();

    if (activeLayer && !activeLayer->isLocked()) {
        SDL_Renderer* renderer = canvas.getRenderer();
        SDL_SetRenderTarget(renderer, activeLayer->getTexture());

        // Set color
        SDL_SetRenderDrawColor(renderer,
            static_cast<Uint8>(m_color.x * 255),
            static_cast<Uint8>(m_color.y * 255),
            static_cast<Uint8>(m_color.z * 255),
            static_cast<Uint8>(m_color.w * 255));

        // Calculate proper triangle points
        int x1 = static_cast<int>(m_startPos.x);
        int y1 = static_cast<int>(m_startPos.y);
        int x2 = static_cast<int>(m_currentPos.x);
        int y2 = static_cast<int>(m_currentPos.y);

        // Third point: center of base horizontally, height based on base width
        int baseWidth = abs(x2 - x1);
        int x3 = (x1 + x2) / 2;
        int y3 = y1 - static_cast<int>(baseWidth * 0.866f);

        // Draw triangle with thickness like line tool
        for (int i = 0; i < m_size; i++) {
            int offset = i - m_size/2;
            // Draw three connected lines with proper thickness
            SDL_RenderDrawLine(renderer, x1 + offset, y1, x3 + offset, y3);
            SDL_RenderDrawLine(renderer, x3 + offset, y3, x2 + offset, y2);
            SDL_RenderDrawLine(renderer, x2 + offset, y2, x1 + offset, y1);
            // Additional thickness in perpendicular direction
            SDL_RenderDrawLine(renderer, x1, y1 + offset, x3, y3 + offset);
            SDL_RenderDrawLine(renderer, x3, y3 + offset, x2, y2 + offset);
            SDL_RenderDrawLine(renderer, x2, y2 + offset, x1, y1 + offset);
        }

        SDL_SetRenderTarget(renderer, nullptr);
    }

    m_isDrawing = false;
}

void TriangleTool::render(SDL_Renderer* renderer) {
    if (!m_isDrawing) return;

    // Set color
    SDL_SetRenderDrawColor(renderer,
        static_cast<Uint8>(m_color.x * 255),
        static_cast<Uint8>(m_color.y * 255),
        static_cast<Uint8>(m_color.z * 255),
        static_cast<Uint8>(m_color.w * 255));

    // Calculate proper triangle points for preview
    int x1 = static_cast<int>(m_startPos.x);
    int y1 = static_cast<int>(m_startPos.y);
    int x2 = static_cast<int>(m_currentPos.x);
    int y2 = static_cast<int>(m_currentPos.y);

    // Third point: center of base horizontally, height based on base width
    int baseWidth = abs(x2 - x1);
    int x3 = (x1 + x2) / 2;
    int y3 = y1 - static_cast<int>(baseWidth * 0.866f);

    // Draw preview triangle with current thickness
    for (int i = 0; i < m_size; i++) {
        int offset = i - m_size/2;
        SDL_RenderDrawLine(renderer, x1 + offset, y1, x3 + offset, y3);
        SDL_RenderDrawLine(renderer, x3 + offset, y3, x2 + offset, y2);
        SDL_RenderDrawLine(renderer, x2 + offset, y2, x1 + offset, y1);
    }
}

// FillTool implementation
void FillTool::handleMouseDown(const SDL_Event& event) {
    m_isDrawing = true;

    // Get click coordinates
    int x = event.button.x;
    int y = event.button.y;

    // Canvas& canvas = GetCanvas();
    Editor::getInstance().saveUndoState();
    floodFill(x, y, m_color, ImVec4(0, 0, 0, 0));

    m_isDrawing = false;
}

void FillTool::floodFill(int x, int y, ImVec4 fillColor, ImVec4 /* targetColor */) {
    // HACK: old queue-based fill was memory hog (issue #156)
    Canvas& canvas = GetCanvas();
    Layer* activeLayer = canvas.getActiveLayer();

    if (!activeLayer || activeLayer->isLocked()) return;
    SDL_Renderer* renderer = canvas.getRenderer();
    SDL_Texture* texture = activeLayer->getTexture();

    if (!texture) return;

    int width, height;
    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);

    if (x < 0 || x >= width || y < 0 || y >= height) return;

    // Read texture into surface for fast pixel access
    SDL_SetRenderTarget(renderer, texture);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

    if (!surface) {
        SDL_SetRenderTarget(renderer, nullptr);
        return;
    }

    SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch);

    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    Uint32 targetColor = pixels[y * width + x];

    // Convert fill color
    Uint8 r = static_cast<Uint8>(fillColor.x * 255);
    Uint8 g = static_cast<Uint8>(fillColor.y * 255);
    Uint8 b = static_cast<Uint8>(fillColor.z * 255);
    Uint8 a = static_cast<Uint8>(fillColor.w * 255);
    Uint32 newColor = (a << 24) | (b << 16) | (g << 8) | r;

    if (targetColor == newColor) {
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(renderer, nullptr);
        return;
    }

    // Stack-based flood fill - much faster
    std::vector<std::pair<int, int>> stack;
    stack.push_back({x, y});

    while (!stack.empty()) {
        auto [cx, cy] = stack.back();
        stack.pop_back();

        if (cx < 0 || cx >= width || cy < 0 || cy >= height) continue;
        if (pixels[cy * width + cx] != targetColor) continue;

        // Find the leftmost pixel in this row
        int left = cx;
        while (left > 0 && pixels[cy * width + (left - 1)] == targetColor) {
            left--;
        }

        // Find the rightmost pixel in this row
        int right = cx;
        while (right < width - 1 && pixels[cy * width + (right + 1)] == targetColor) {
            right++;
        }

        // Fill the horizontal line
        for (int i = left; i <= right; i++) {
            pixels[cy * width + i] = newColor;
        }

        // Add pixels above and below to the stack
        for (int i = left; i <= right; i++) {
            if (cy > 0 && pixels[(cy - 1) * width + i] == targetColor) {
                stack.push_back({i, cy - 1});
            }
            if (cy < height - 1 && pixels[(cy + 1) * width + i] == targetColor) {
                stack.push_back({i, cy + 1});
            }
        }
    }

    // Copy back to texture
    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (newTexture) {
        SDL_RenderCopy(renderer, newTexture, nullptr, nullptr);
        SDL_DestroyTexture(newTexture);
    }

    SDL_SetRenderTarget(renderer, nullptr);
}

// SelectionTool implementation
void SelectionTool::handleMouseDown(const SDL_Event& event) {
    // COMMENTED OUT OLD IMPLEMENTATION - REPLACED WITH CLEANER VERSION
    /*
    m_isDrawing = true;
    m_startPos = ImVec2(event.button.x, event.button.y);
    m_currentPos = m_startPos;

    Canvas& canvas = GetCanvas();

    // Use smart object selection to find and select layer at click point
    canvas.selectLayerAtPoint(event.button.x, event.button.y);

    // FIXED: Only handle transform operations when shift is held
    // Regular left click = just selection, no transform operations
    SDL_Point mousePos = {event.button.x, event.button.y};

    if (event.button.button == SDL_BUTTON_LEFT) {
        const Uint8* keyState = SDL_GetKeyboardState(nullptr);
        if (keyState[SDL_SCANCODE_LSHIFT] || keyState[SDL_SCANCODE_RSHIFT]) {
            // Only do transform operations when shift is held
            canvas.handleTransformDrag(event, mousePos);
        }
        // Regular left click does nothing - just selection
    }
    // Removed problematic right-click handling that caused unwanted scaling

    // Clear old selection system if no transform is active
    if (!canvas.isTransformBoxVisible()) {
        canvas.setHasSelection(false);
        canvas.setSelectionRect({0, 0, 0, 0});
        if (canvas.getSelectionTexture()) {
            SDL_DestroyTexture(canvas.getSelectionTexture());
            canvas.setSelectionTexture(nullptr);
        }
    }
    */

    // NEW IMPLEMENTATION: Clean separation of move vs scale operations
    if (event.button.button != SDL_BUTTON_LEFT) return;

    m_isDrawing = true;
    m_startPos = ImVec2(event.button.x, event.button.y);
    m_currentPos = m_startPos;

    Canvas& canvas = GetCanvas();

    // Always select layer at click point first
    canvas.selectLayerAtPoint(event.button.x, event.button.y);

    // Check if shift is held - this determines the operation mode
    const Uint8* keyState = SDL_GetKeyboardState(nullptr);
    bool shiftHeld = (keyState[SDL_SCANCODE_LSHIFT] || keyState[SDL_SCANCODE_RSHIFT]);

    if (shiftHeld) {
        // SHIFT+LEFT CLICK: Scale mode - higher precedence
        if (canvas.isTransformBoxVisible()) {
            SDL_Point mousePos = {event.button.x, event.button.y};
            canvas.handleTransformDrag(event, mousePos);
        }
    } else {
        // LEFT CLICK ONLY: Move mode - only if inside transform box
        if (canvas.isTransformBoxVisible()) {
            SDL_Point mousePos = {event.button.x, event.button.y};
            SDL_Rect transformRect = canvas.getTransformRect();

            // Check if click is inside the transform box
            if (mousePos.x >= transformRect.x && mousePos.x <= transformRect.x + transformRect.w &&
                mousePos.y >= transformRect.y && mousePos.y <= transformRect.y + transformRect.h) {
                canvas.handleTransformDrag(event, mousePos);
            }
        }
    }
}

void SelectionTool::handleMouseMove(const SDL_Event& event) {
    if (!m_isDrawing) return;

    m_currentPos = ImVec2(event.motion.x, event.motion.y);

    Canvas& canvas = GetCanvas();

    // Only handle transform if we have a visible transform box
    if (canvas.isTransformBoxVisible()) {
        SDL_Point mousePos = {event.motion.x, event.motion.y};
        SDL_Event motionEvent = event;
        canvas.handleTransformDrag(motionEvent, mousePos);
    }
}

void SelectionTool::handleMouseUp(const SDL_Event& event) {
    if (!m_isDrawing) return;

    Canvas& canvas = GetCanvas();

    // Always handle transform completion if transform box is visible
    if (canvas.isTransformBoxVisible()) {
        SDL_Point mousePos = {event.button.x, event.button.y};
        canvas.handleTransformDrag(event, mousePos);
    } else {
        // Fallback to rectangular selection if no layer was selected
        int x = std::min(static_cast<int>(m_startPos.x), static_cast<int>(m_currentPos.x));
        int y = std::min(static_cast<int>(m_startPos.y), static_cast<int>(m_currentPos.y));
        int w = std::abs(static_cast<int>(m_currentPos.x - m_startPos.x));
        int h = std::abs(static_cast<int>(m_currentPos.y - m_startPos.y));

        // Only create selection if it has reasonable area
        if (w > 5 && h > 5) {
            SDL_Rect selectionRect = {x, y, w, h};
            canvas.setSelectionRect(selectionRect);
            canvas.setHasSelection(true);

            // Create selection texture
            Editor::getInstance().copySelection();
        }
    }

    m_isDrawing = false;
}

void SelectionTool::render(SDL_Renderer* renderer) {
    if (!m_isDrawing) return;

    // Draw selection preview
    SDL_SetRenderDrawColor(renderer, 0, 120, 215, 128);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Calculate rectangle dimensions
    int x = std::min(static_cast<int>(m_startPos.x), static_cast<int>(m_currentPos.x));
    int y = std::min(static_cast<int>(m_startPos.y), static_cast<int>(m_currentPos.y));
    int w = std::abs(static_cast<int>(m_currentPos.x - m_startPos.x));
    int h = std::abs(static_cast<int>(m_currentPos.y - m_startPos.y));

    SDL_Rect rect = {x, y, w, h};
    SDL_RenderDrawRect(renderer, &rect);

    // Draw dashed lines
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);

    const int dashLength = 4;
    for (int i = x; i < x + w; i += dashLength * 2) {
        SDL_Rect dashRect = {i, y, dashLength, 1};
        SDL_RenderFillRect(renderer, &dashRect);
        dashRect.y = y + h - 1;
        SDL_RenderFillRect(renderer, &dashRect);
    }

    for (int i = y; i < y + h; i += dashLength * 2) {
        SDL_Rect dashRect = {x, i, 1, dashLength};
        SDL_RenderFillRect(renderer, &dashRect);
        dashRect.x = x + w - 1;
        SDL_RenderFillRect(renderer, &dashRect);
    }
}

void SelectionTool::cancel() {
    m_isDrawing = false;

    Canvas& canvas = GetCanvas();
    if (canvas.hasSelection()) {
        canvas.setHasSelection(false);
        canvas.setSelectionRect({0, 0, 0, 0});
        if (canvas.getSelectionTexture()) {
            SDL_DestroyTexture(canvas.getSelectionTexture());
            canvas.setSelectionTexture(nullptr);
        }
    }
}

// Flood selection tool implementation - directly inspired by flood fill algorithm
void FloodSelectionTool::handleMouseDown(const SDL_Event& event) {
    m_isDrawing = true;

    Canvas& canvas = GetCanvas();
    Layer* activeLayer = canvas.getActiveLayer();

    if (!activeLayer) return;

    int x = event.button.x;
    int y = event.button.y;

    SDL_Renderer* renderer = canvas.getRenderer();
    SDL_Texture* texture = activeLayer->getTexture();

    if (!texture) return;

    // Read pixel color at click point
    SDL_SetRenderTarget(renderer, texture);

    int width, height;
    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);

    if (x < 0 || x >= width || y < 0 || y >= height) {
        SDL_SetRenderTarget(renderer, nullptr);
        return;
    }

    // Create surface to read pixel data
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

    if (!surface) {
        SDL_SetRenderTarget(renderer, nullptr);
        return;
    }

    SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch);

    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    Uint32 targetColorRaw = pixels[y * width + x];

    // Convert to ImVec4 for comparison
    ImVec4 targetColor = {
        ((targetColorRaw & 0xFF) / 255.0f),
        (((targetColorRaw >> 8) & 0xFF) / 255.0f),
        (((targetColorRaw >> 16) & 0xFF) / 255.0f),
        (((targetColorRaw >> 24) & 0xFF) / 255.0f)
    };

    SDL_FreeSurface(surface);
    SDL_SetRenderTarget(renderer, nullptr);

    clearSelection();
    floodSelect(x, y, targetColor);

    m_isDrawing = false;
}

void FloodSelectionTool::handleMouseMove(const SDL_Event& /* event */) {
    // No operation for flood selection
}

void FloodSelectionTool::handleMouseUp(const SDL_Event& /* event */) {
    // No operation for flood selection
}

void FloodSelectionTool::render(SDL_Renderer* renderer) {
    if (m_selectedPixels.empty()) return;

    // Draw selection as highlighted pixels
    SDL_SetRenderDrawColor(renderer, 0, 120, 215, 128);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (const auto& pixel : m_selectedPixels) {
        SDL_Rect pixelRect = {pixel.x, pixel.y, 1, 1};
        SDL_RenderFillRect(renderer, &pixelRect);
    }
}

void FloodSelectionTool::cancel() {
    clearSelection();
}

void FloodSelectionTool::floodSelect(int x, int y, ImVec4 targetColor) {
    Canvas& canvas = GetCanvas();
    Layer* activeLayer = canvas.getActiveLayer();

    if (!activeLayer) return;

    SDL_Renderer* renderer = canvas.getRenderer();
    SDL_Texture* texture = activeLayer->getTexture();

    if (!texture) return;

    int width, height;
    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);

    if (x < 0 || x >= width || y < 0 || y >= height) return;

    // Read texture into surface for pixel access
    SDL_SetRenderTarget(renderer, texture);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

    if (!surface) {
        SDL_SetRenderTarget(renderer, nullptr);
        return;
    }

    SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch);

    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));

    // Stack-based flood selection algorithm - inspired by flood fill
    std::vector<std::pair<int, int>> stack;
    stack.push_back({x, y});

    while (!stack.empty()) {
        auto [cx, cy] = stack.back();
        stack.pop_back();

        if (cx < 0 || cx >= width || cy < 0 || cy >= height) continue;
        if (visited[cy][cx]) continue;

        Uint32 currentColorRaw = pixels[cy * width + cx];
        ImVec4 currentColor = {
            ((currentColorRaw & 0xFF) / 255.0f),
            (((currentColorRaw >> 8) & 0xFF) / 255.0f),
            (((currentColorRaw >> 16) & 0xFF) / 255.0f),
            (((currentColorRaw >> 24) & 0xFF) / 255.0f)
        };

        if (!isColorSimilar(currentColor, targetColor)) continue;

        visited[cy][cx] = true;
        m_selectedPixels.push_back({cx, cy});

        // Add neighboring pixels to stack
        stack.push_back({cx - 1, cy});
        stack.push_back({cx + 1, cy});
        stack.push_back({cx, cy - 1});
        stack.push_back({cx, cy + 1});
    }

    SDL_FreeSurface(surface);
    SDL_SetRenderTarget(renderer, nullptr);

    // Create selection rect from selected pixels
    if (!m_selectedPixels.empty()) {
        int minX = width, maxX = 0;
        int minY = height, maxY = 0;

        for (const auto& pixel : m_selectedPixels) {
            minX = std::min(minX, pixel.x);
            maxX = std::max(maxX, pixel.x);
            minY = std::min(minY, pixel.y);
            maxY = std::max(maxY, pixel.y);
        }

        SDL_Rect selectionRect = {minX, minY, maxX - minX + 1, maxY - minY + 1};
        canvas.setSelectionRect(selectionRect);
        canvas.setHasSelection(true);
    }
}

bool FloodSelectionTool::isColorSimilar(ImVec4 color1, ImVec4 color2) const {
    float tolerance = m_tolerance / 255.0f;

    return (std::abs(color1.x - color2.x) <= tolerance &&
            std::abs(color1.y - color2.y) <= tolerance &&
            std::abs(color1.z - color2.z) <= tolerance &&
            std::abs(color1.w - color2.w) <= tolerance);
}

void FloodSelectionTool::clearSelection() {
    m_selectedPixels.clear();

    Canvas& canvas = GetCanvas();
    canvas.setHasSelection(false);
    canvas.setSelectionRect({0, 0, 0, 0});
    if (canvas.getSelectionTexture()) {
        SDL_DestroyTexture(canvas.getSelectionTexture());
        canvas.setSelectionTexture(nullptr);
    }
}

void FloodSelectionTool::deleteSelectedPixels() {
    if (m_selectedPixels.empty()) return;

    Canvas& canvas = GetCanvas();
    Layer* activeLayer = canvas.getActiveLayer();

    if (!activeLayer || activeLayer->isLocked()) return;

    // Save undo state before destructive operation
    Editor::getInstance().saveUndoState();

    SDL_Renderer* renderer = canvas.getRenderer();
    SDL_Texture* texture = activeLayer->getTexture();

    if (!texture) return;

    int width, height;
    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);

    // Read texture into surface for pixel manipulation
    SDL_SetRenderTarget(renderer, texture);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

    if (!surface) {
        SDL_SetRenderTarget(renderer, nullptr);
        return;
    }

    SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch);

    Uint32* pixels = static_cast<Uint32*>(surface->pixels);

    // Delete selected pixels by making them transparent
    // (Enough is cute - but deletion is serious business)
    for (const auto& pixel : m_selectedPixels) {
        if (pixel.x >= 0 && pixel.x < width && pixel.y >= 0 && pixel.y < height) {
            pixels[pixel.y * width + pixel.x] = 0x00000000; // Transparent
        }
    }

    // Create new texture from modified surface
    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_FreeSurface(surface);
    SDL_SetRenderTarget(renderer, nullptr);

    if (newTexture) {
        SDL_SetTextureBlendMode(newTexture, SDL_BLENDMODE_BLEND);
        activeLayer->setTexture(newTexture);
    }

    // Clear selection after deletion
    clearSelection();
}

// TextTool implementation
TextTool::TextTool() : m_activeTextBox(-1), m_needsUpdate(false) {
    m_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    // NOTE: font loading crashes on some systems - moved to lazy init
    loadAvailableFonts();
}

TextTool::~TextTool() {
    for (auto& textBox : m_textBoxes) {
        if (textBox.isActive) {
            renderTextBoxToLayer(textBox);
        }
    }
    m_textBoxes.clear();
}

void TextTool::handleMouseDown(const SDL_Event& event) {
    if (event.button.button != SDL_BUTTON_LEFT) return;

    int x = event.button.x;
    int y = event.button.y;

    Canvas& canvas = GetCanvas();
    (void)canvas;

    for (size_t i = 0; i < m_textBoxes.size(); i++) {
        SDL_Rect rect = m_textBoxes[i].rect;
        if (x >= rect.x && x < rect.x + rect.w &&
            y >= rect.y && y < rect.y + rect.h) {
            activateTextBox(i);
            return;
        }
    }

    m_isDrawing = true;
    m_startPos = ImVec2(x, y);
    m_currentPos = m_startPos;
    Editor::getInstance().saveUndoState();
}

void TextTool::handleMouseMove(const SDL_Event& event) {
    if (!m_isDrawing) return;
    m_currentPos = ImVec2(event.motion.x, event.motion.y);
}

void TextTool::handleMouseUp(const SDL_Event& event) {
    (void)event;
    if (!m_isDrawing) return;

    int x = std::min(static_cast<int>(m_startPos.x), static_cast<int>(m_currentPos.x));
    int y = std::min(static_cast<int>(m_startPos.y), static_cast<int>(m_currentPos.y));
    int w = std::abs(static_cast<int>(m_currentPos.x - m_startPos.x));
    int h = std::abs(static_cast<int>(m_currentPos.y - m_startPos.y));

    if (w < 20) w = 200;
    if (h < 20) h = 50;

    createTextBox(x, y, w, h);
    m_isDrawing = false;
}

void TextTool::render(SDL_Renderer* renderer) {
    renderTextBoxes(renderer);

    if (m_isDrawing) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        int x = std::min(static_cast<int>(m_startPos.x), static_cast<int>(m_currentPos.x));
        int y = std::min(static_cast<int>(m_startPos.y), static_cast<int>(m_currentPos.y));
        int w = std::abs(static_cast<int>(m_currentPos.x - m_startPos.x));
        int h = std::abs(static_cast<int>(m_currentPos.y - m_startPos.y));

        if (w < 20) w = 200;
        if (h < 20) h = 50;

        SDL_Rect rect = {x, y, w, h};
        SDL_RenderDrawRect(renderer, &rect);
    }
}

void TextTool::createTextBox(int x, int y, int w, int h) {
    Canvas& canvas = GetCanvas();
    std::string layerName = "Text " + std::to_string(m_textBoxes.size() + 1);
    canvas.addLayer(layerName);
    // WORKAROUND: should reuse layers but merge conflicts with v1.1 branch

    TextBox newBox;
    newBox.rect = {x, y, w, h};
    newBox.content = "Enter text here";
    newBox.isActive = true;
    newBox.color = m_color;
    newBox.fontSize = 24;
    newBox.bold = false;
    newBox.italic = false;
    newBox.layerIndex = canvas.getLayers().size() - 1;
    newBox.fontPath = "";
    newBox.fontName = "Default";

    deactivateAllTextBoxes();
    m_textBoxes.push_back(newBox);
    m_activeTextBox = m_textBoxes.size() - 1;
    m_needsUpdate = true;
}

void TextTool::activateTextBox(int index) {
    if (index < 0 || index >= static_cast<int>(m_textBoxes.size())) return;

    deactivateAllTextBoxes();
    m_activeTextBox = index;
    m_textBoxes[index].isActive = true;
    m_needsUpdate = true;
}

void TextTool::deactivateAllTextBoxes() {
    for (auto& box : m_textBoxes) {
        if (box.isActive) {
            box.isActive = false;
            m_needsUpdate = true;
        }
    }
    m_activeTextBox = -1;
}

void TextTool::finalizeTextBox(int index) {
    if (index < 0 || index >= static_cast<int>(m_textBoxes.size())) return;

    m_textBoxes[index].isActive = false;
    renderTextBoxToLayer(m_textBoxes[index]);
    m_needsUpdate = true;
}

void TextTool::deleteTextBox(int index) {
    if (index < 0 || index >= static_cast<int>(m_textBoxes.size())) return;

    m_textBoxes.erase(m_textBoxes.begin() + index);
    if (m_activeTextBox == index) {
        m_activeTextBox = -1;
    } else if (m_activeTextBox > index) {
        m_activeTextBox--;
    }
    m_needsUpdate = true;
}

void TextTool::renderTextBoxToLayer(const TextBox& textBox) {
    Canvas& canvas = GetCanvas();
    const auto& layers = canvas.getLayers();

    if (textBox.layerIndex < 0 || textBox.layerIndex >= static_cast<int>(layers.size())) {
        // std::cout << "Bad layer index: " << textBox.layerIndex << std::endl;
        return;
    }

    SDL_Renderer* renderer = canvas.getRenderer();
    Layer* targetLayer = layers[textBox.layerIndex].get();

    if (!targetLayer || targetLayer->isLocked()) {
        return;
    }

    TTF_Font* font = nullptr;
    if (!textBox.fontPath.empty()) {
        font = TTF_OpenFont(textBox.fontPath.c_str(), textBox.fontSize);
    }

    if (!font) {
        font = canvas.getFont(textBox.fontSize, textBox.bold, textBox.italic);
    }

    if (!font) return;

    int fontStyle = TTF_STYLE_NORMAL;
    if (textBox.bold) fontStyle |= TTF_STYLE_BOLD;
    if (textBox.italic) fontStyle |= TTF_STYLE_ITALIC;
    TTF_SetFontStyle(font, fontStyle);

    SDL_Color textColor = {
        static_cast<Uint8>(textBox.color.x * 255),
        static_cast<Uint8>(textBox.color.y * 255),
        static_cast<Uint8>(textBox.color.z * 255),
        static_cast<Uint8>(textBox.color.w * 255)
    };

    SDL_Surface* textSurface = TTF_RenderText_Blended_Wrapped(
        font,
        textBox.content.c_str(),
        textColor,
        textBox.rect.w
    );

    if (!textSurface) {
        if (!textBox.fontPath.empty() && font != canvas.getFont(textBox.fontSize, textBox.bold, textBox.italic)) {
            TTF_CloseFont(font);
        }
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);

    if (!textTexture) {
        if (!textBox.fontPath.empty() && font != canvas.getFont(textBox.fontSize, textBox.bold, textBox.italic)) {
            TTF_CloseFont(font);
        }
        return;
    }

    SDL_SetRenderTarget(renderer, targetLayer->getTexture());
    SDL_Rect destRect = textBox.rect;
    SDL_RenderCopy(renderer, textTexture, nullptr, &destRect);
    SDL_DestroyTexture(textTexture);
    SDL_SetRenderTarget(renderer, nullptr);

    if (!textBox.fontPath.empty() && font != canvas.getFont(textBox.fontSize, textBox.bold, textBox.italic)) {
        TTF_CloseFont(font);
    }
}

void TextTool::renderTextBoxes(SDL_Renderer* renderer) {
    for (const auto& box : m_textBoxes) {
        renderTextBoxPreview(renderer, box);
        drawTextBoxBorder(renderer, box, box.isActive);
    }
}



void TextTool::renderTextBoxPreview(SDL_Renderer* renderer, const TextBox& textBox) {
    Canvas& canvas = GetCanvas();

    TTF_Font* font = nullptr;
    if (!textBox.fontPath.empty()) {
        font = TTF_OpenFont(textBox.fontPath.c_str(), textBox.fontSize);
    }

    if (!font) {
        font = canvas.getFont(textBox.fontSize, textBox.bold, textBox.italic);
    }

    if (!font) return;

    int fontStyle = TTF_STYLE_NORMAL;
    if (textBox.bold) fontStyle |= TTF_STYLE_BOLD;
    if (textBox.italic) fontStyle |= TTF_STYLE_ITALIC;
    TTF_SetFontStyle(font, fontStyle);

    SDL_Color textColor = {
        static_cast<Uint8>(textBox.color.x * 255),
        static_cast<Uint8>(textBox.color.y * 255),
        static_cast<Uint8>(textBox.color.z * 255),
        static_cast<Uint8>(textBox.color.w * 255)
    };

    SDL_Surface* textSurface = TTF_RenderText_Blended_Wrapped(
        font,
        textBox.content.c_str(),
        textColor,
        textBox.rect.w
    );

    if (!textSurface) {
        if (!textBox.fontPath.empty() && font != canvas.getFont(textBox.fontSize, textBox.bold, textBox.italic)) {
            TTF_CloseFont(font);
        }
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);

    if (!textTexture) {
        if (!textBox.fontPath.empty() && font != canvas.getFont(textBox.fontSize, textBox.bold, textBox.italic)) {
            TTF_CloseFont(font);
        }
        return;
    }

    SDL_Rect destRect = textBox.rect;
    SDL_RenderCopy(renderer, textTexture, nullptr, &destRect);
    SDL_DestroyTexture(textTexture);

    if (!textBox.fontPath.empty() && font != canvas.getFont(textBox.fontSize, textBox.bold, textBox.italic)) {
        TTF_CloseFont(font);
    }
}

// GradientTool implementation
void GradientTool::handleMouseDown(const SDL_Event& event) {
    m_isDrawing = true;
    m_startPos = ImVec2(event.button.x, event.button.y);
    m_currentPos = m_startPos;

    // Save state for undo
    Editor::getInstance().saveUndoState();
}

void GradientTool::handleMouseMove(const SDL_Event& event) {
    if (!m_isDrawing) return;
    m_currentPos = ImVec2(event.motion.x, event.motion.y);
}

void GradientTool::handleMouseUp(const SDL_Event&) {
    if (!m_isDrawing) return;

    Canvas& canvas = GetCanvas();
    Layer* activeLayer = canvas.getActiveLayer();

    if (activeLayer && !activeLayer->isLocked()) {
        SDL_Renderer* renderer = canvas.getRenderer();
        SDL_SetRenderTarget(renderer, activeLayer->getTexture());

        // Draw the final gradient
        drawGradient(renderer, m_startPos, m_currentPos, m_color, m_secondaryColor);

        SDL_SetRenderTarget(renderer, nullptr);
    }

    m_isDrawing = false;
}

void GradientTool::render(SDL_Renderer* renderer) {
    if (!m_isDrawing) return;

    // Draw gradient preview
    drawGradient(renderer, m_startPos, m_currentPos, m_color, m_secondaryColor);
}

void GradientTool::drawGradient(SDL_Renderer* renderer, ImVec2 start, ImVec2 end, ImVec4 startColor, ImVec4 endColor) {
    // Get gradient parameters
    float dx = end.x - start.x;
        float dy = end.y - start.y;
        float distance = std::sqrt(dx*dx + dy*dy);

        // Skip if distance is too small
        if (distance < 1.0f) return;

        // float nx = dx / distance;
        // float ny = dy / distance;


        int minX = std::min(static_cast<int>(start.x), static_cast<int>(end.x));
        int maxX = std::max(static_cast<int>(start.x), static_cast<int>(end.x));
        int minY = std::min(static_cast<int>(start.y), static_cast<int>(end.y));
        int maxY = std::max(static_cast<int>(start.y), static_cast<int>(end.y));

        // Make sure to have some reasonable bounds
        if (maxX - minX < 2) { minX -= 100; maxX += 100; }
        if (maxY - minY < 2) { minY -= 100; maxY += 100; }

        // Render based on gradient type
        switch (m_type) {
            case GradientType::LINEAR: {
                // Linear gradient along the line
                for (int y = minY; y <= maxY; y++) {
                    for (int x = minX; x <= maxX; x++) {
                        float projDist;
                        if (std::abs(dx) > std::abs(dy)) {
                            // Primarily horizontal
                            projDist = (x - start.x) / dx;
                        } else {
                            // Primarily vertical
                            projDist = (y - start.y) / dy;
                        }

                        // Clamp to [0, 1]
                        projDist = std::max(0.0f, std::min(1.0f, projDist));

                        // Interpolate color
                        Uint8 r = static_cast<Uint8>((startColor.x * (1.0f - projDist) + endColor.x * projDist) * 255);
                        Uint8 g = static_cast<Uint8>((startColor.y * (1.0f - projDist) + endColor.y * projDist) * 255);
                        Uint8 b = static_cast<Uint8>((startColor.z * (1.0f - projDist) + endColor.z * projDist) * 255);
                        Uint8 a = static_cast<Uint8>((startColor.w * (1.0f - projDist) + endColor.w * projDist) * 255);

                        SDL_SetRenderDrawColor(renderer, r, g, b, a);
                        SDL_RenderDrawPoint(renderer, x, y);
                    }
                }
                break;
            }

            case GradientType::RADIAL: {
                // Radial gradient from start to end
                float maxDist = distance;
                for (int y = minY; y <= maxY; y++) {
                    for (int x = minX; x <= maxX; x++) {
                        float dx = x - start.x;
                        float dy = y - start.y;
                        float dist = std::sqrt(dx*dx + dy*dy);

                        // Normalize distance to [0, 1]
                        float t = std::min(dist / maxDist, 1.0f);

                        // Interpolate color
                        Uint8 r = static_cast<Uint8>((startColor.x * (1.0f - t) + endColor.x * t) * 255);
                        Uint8 g = static_cast<Uint8>((startColor.y * (1.0f - t) + endColor.y * t) * 255);
                        Uint8 b = static_cast<Uint8>((startColor.z * (1.0f - t) + endColor.z * t) * 255);
                        Uint8 a = static_cast<Uint8>((startColor.w * (1.0f - t) + endColor.w * t) * 255);

                        SDL_SetRenderDrawColor(renderer, r, g, b, a);
                        SDL_RenderDrawPoint(renderer, x, y);
                    }
                }
                break;
            }

            case GradientType::ANGULAR: {
                // Angular gradient around start point
                float startAngle = std::atan2(dy, dx);
                for (int y = minY; y <= maxY; y++) {
                    for (int x = minX; x <= maxX; x++) {
                        float dx = x - start.x;
                        float dy = y - start.y;

                        // Skip the center point to avoid division by zero
                        if (std::abs(dx) < 0.01f && std::abs(dy) < 0.01f) continue;

                        // Calculate angle
                        float angle = std::atan2(dy, dx);

                        // Normalize angle difference to [0, 1]
                        float angleDiff = angle - startAngle;
                        if (angleDiff < 0) angleDiff += 2 * M_PI;
                        float t = angleDiff / (2 * M_PI);

                        // Interpolate color
                        Uint8 r = static_cast<Uint8>((startColor.x * (1.0f - t) + endColor.x * t) * 255);
                        Uint8 g = static_cast<Uint8>((startColor.y * (1.0f - t) + endColor.y * t) * 255);
                        Uint8 b = static_cast<Uint8>((startColor.z * (1.0f - t) + endColor.z * t) * 255);
                        Uint8 a = static_cast<Uint8>((startColor.w * (1.0f - t) + endColor.w * t) * 255);

                        SDL_SetRenderDrawColor(renderer, r, g, b, a);
                        SDL_RenderDrawPoint(renderer, x, y);
                    }
                }
                break;
            }
        }
    }

// HealingTool implementation
void HealingTool::handleMouseDown(const SDL_Event& event) {
    m_isDrawing = true;
    m_startPos = ImVec2(event.button.x, event.button.y);
    m_currentPos = m_startPos;

    // Apply healing at the first position
    // Save state for undo
    Editor::getInstance().saveUndoState();

    // Apply healing at the initial position
    int x = static_cast<int>(m_startPos.x);
    int y = static_cast<int>(m_startPos.y);
    applyHealingAt(x, y);
}
void HealingTool::handleMouseMove(const SDL_Event& event) {
    if (!m_isDrawing) return;

    ImVec2 newPos(event.motion.x, event.motion.y);

    // Apply healing along the path
    float dx = newPos.x - m_currentPos.x;
    float dy = newPos.y - m_currentPos.y;
    float distance = std::sqrt(dx*dx + dy*dy);

    if (distance > 0) {
        // Interpolate points for smooth healing
        float step = 1.0f / distance;
        for (float t = 0; t <= 1.0f; t += step) {
            int x = static_cast<int>(m_currentPos.x + dx * t);
            int y = static_cast<int>(m_currentPos.y + dy * t);
            applyHealingAt(x, y);
        }
    }

    m_currentPos = newPos;
}

void HealingTool::handleMouseUp(const SDL_Event&) {
    m_isDrawing = false;
}

void TextTool::loadAvailableFonts() {
    m_availableFonts.clear();
    m_fontNames.clear();

    // CRUDE: hardcode arial path since font discovery is flaky
    m_availableFonts.push_back("arial.ttf");
    m_fontNames.push_back("Default (Arial)");

    scanFontDirectory("fonts/");
    scanFontDirectory("/usr/share/fonts/truetype/");
    scanFontDirectory("/System/Library/Fonts/");
    scanFontDirectory("C:/Windows/Fonts/");
}

void TextTool::scanFontDirectory(const std::string& directory) {
    if (directory == "fonts/") {
        // HACK: directory scanning broken on Windows - use hardcoded list
        std::vector<std::string> knownFonts = {
            "!The Black Bloc Bold.ttf", "!The Black Bloc Regular.ttf",
            "08 Underground.ttf", "1942 Report.ttf", "36 Days Ago Bold.ttf",
            "36 Days Ago Regular.ttf", "50 Blizzards.ttf", "8 Bit Wonder.ttf",
            "A Dripping Marker.ttf", "Aaaiight! Bold.ttf", "Aaaiight! Regular.ttf",
            "Aardvark Cafe.ttf", "Adolphus Serif.ttf", "Adolphus.ttf",
            "African.ttf", "Agafont.ttf", "Airmole Antique.ttf",
            "Airmole Regular.ttf", "Alanus.ttf", "Aldo.ttf",
            "All Hooked Up.ttf", "Almonte.ttf", "Alpha 54.ttf",
            "Alphabits Fat.ttf", "Alphabits Regular.ttf", "Amerika Sans.ttf",
            "Antelope.ttf", "Arabolical.ttf", "Archeologicaps.ttf",
            "Architek.ttf", "Art Brush.ttf", "Artistica.ttf",
            "Astrud.ttf", "Augusta Shadow.ttf", "Augusta.ttf",
            "Averia Bold.ttf", "Averia Regular.ttf", "Averia Serif Regular.ttf",
            "B-Boy.ttf", "Bad Boys.ttf", "Bajenna.ttf",
            "Baldur Shadow.ttf", "Baldur.ttf", "Beagle Brigade.ttf",
            "Benegraphic.ttf", "Berlin Allee.ttf", "Berylium Regular.ttf",
            "Beta 54.ttf", "Big Head.ttf", "Big Mummy.ttf",
            "Black Jack.ttf", "Blax Slab XXL.ttf", "Blues MK2.ttf",
            "Bodonitown.ttf", "Bogotana Regular.ttf", "Bogusflow.ttf",
            "Bolonewt.ttf", "Bones Font.ttf", "Bonzer San Francisco.ttf",
            "Borg.ttf", "Boston Traffic.ttf", "Breamcatcher.ttf",
            "Brush Of Kent.ttf", "Bud Null.ttf", "Bullpen HV.ttf",
            "Caligula Dodgy.ttf", "Carbon Blade.ttf", "CarbonType.ttf",
            "Carbona.ttf", "Cardinal.ttf", "Cargo Crate.ttf",
            "Cat Shop.ttf", "Chang  And Eng.ttf", "Chantelli Antiqua.ttf",
            "Chinese Calligraphy.ttf", "Chisel Script.ttf", "Chow Fun.ttf",
            "Chrome Yellow.ttf", "Class A.ttf", "Cleopatra.ttf",
            "Cliff Edge.ttf", "Colourbars.ttf", "Concrete Shoes.ttf",
            "Corleone.ttf", "Corporate HQ.ttf", "Crack.ttf",
            "Crazy Crazy.ttf", "Credit River.ttf", "Crop Types.ttf",
            "Crystal Radio Kit.ttf", "Currency Regular.ttf", "DJ Gross.ttf",
            "Damaged.ttf", "Delitsch Antiqua.ttf", "Deng Thick.ttf",
            "Deutsch Gothic.ttf", "Digicity.ttf", "Digit.ttf",
            "Diogenes.ttf", "Discoid.ttf", "Display Free TFB.ttf",
            "Dominican.ttf", "Don Aquarel.ttf", "Doughnut Monster.ttf",
            "Dream Orphans.ttf", "Duality.ttf", "Duvall.ttf",
            "East Market.ttf", "Echelon.ttf", "Edmunds.ttf",
            "Edo SZ.ttf", "Eight One.ttf", "Eirik Raude.ttf",
            "Elementary Gothic Bookhand.ttf", "Episode 1.ttf", "Epistolar.ttf",
            "Epitough.ttf", "Ethnocentric Regular.ttf", "Etobicoke.ttf",
            "Euphorigenic.ttf", "Expresiva.ttf", "Express.ttf",
            "Expressway.ttf", "Fairfax Station.ttf", "Fashion Victim.ttf",
            "Fat Wedge.ttf", "Felt Pen.ttf", "Festival Jomfruer.ttf",
            "Fette Mikado.ttf", "Fine Stencil.ttf", "Fixxed.ttf",
            "Flow.ttf", "Flux Architect Regular.ttf", "Folks Normal.ttf",
            "Font Penetration.ttf", "Font Shui.ttf", "Foo.ttf",
            "Forgotten Futurist.ttf", "Forgotten.ttf", "Forty Script.ttf",
            "Fountain.ttf", "Friday.ttf", "Game Plan.ttf",
            "Gang Of Three.ttf", "Gaps.ttf", "Gartentika.ttf",
            "Gismonda FG.ttf", "Goma Western.ttf", "Gomo.ttf",
            "Goodfish Regular.ttf", "Gooooly.ttf", "Goth Goma.ttf",
            "Gothic 45.ttf", "Graffiti Font.ttf", "Graffiti Poster.ttf",
            "Graffont.ttf", "Gramophone NF.ttf", "Graphic CAT.ttf",
            "Grass.ttf", "Grave Digger.ttf", "Great Lakes NF.ttf",
            "Greenbeans.ttf", "Hall Fetica.ttf", "Happy Hell.ttf",
            "Hard Edge.ttf", "Heavy Heap.ttf", "Herkules.ttf",
            "High Sans Serif 7.ttf", "Highlander.ttf", "Highway Gothic.ttf",
            "Hit the Road.ttf", "Holla Script.ttf", "Home Remedy.ttf",
            "Hostias.ttf", "Hotel Oriental.ttf", "Humana.ttf",
            "Hydra.ttf", "Ibiza.ttf", "Ice Age.ttf",
            "Icicle Country Two.ttf", "Indira K.ttf", "Infinita.ttf",
            "Inglobal.ttf", "Inky Dinky.ttf", "Inspyratta.ttf",
            "Interact.ttf", "Intuitive.ttf", "Isaac Script 2.ttf",
            "Italexico.ttf", "JD Equinox.ttf", "JSA Lovechinese.ttf",
            "JSL Ancient.ttf", "Jam Pact.ttf", "Jelly.ttf",
            "Jose de Oliveira.ttf", "Juanalzada.ttf", "June Bug Stomp NF.ttf",
            "Jungle Fever.ttf", "Just for Fun.ttf", "KS Brush.ttf",
            "Kana.ttf", "Karate.ttf", "Kells SD.ttf",
            "Kelt Caps Freehand.ttf", "Kelvinized.ttf", "Kenyan Coffee.ttf",
            "Kilsonburg.ttf", "Kimberley BL.ttf", "Kingthings Exeter.ttf",
            "Kingthings Petrock.ttf", "Kingthings Sans.ttf", "Kirsty Regular.ttf",
            "Kitchen.ttf", "Know Your Product.ttf", "Konfuciuz.ttf",
            "Kong.ttf", "Korean Calligraphy.ttf", "Kremlin Comrade.ttf",
            "Kshandwrt.ttf", "La Mamucha.ttf", "La Unica.ttf",
            "Landsdowne.ttf", "Lansbury FG.ttf", "Lasso Of Truth.ttf",
            "Leo Arrow.ttf", "Lesser Concern.ttf", "Lickspittle.ttf",
            "Lignum Melle.ttf", "Livingstone.ttf", "Logobloqo 2.ttf",
            "London MM.ttf", "Lord Juusai.ttf", "Lowery Regular.ttf",
            "Lunch.ttf", "Lupinus.ttf", "Luxembourg 1910.ttf",
            "MCapitals.ttf", "MK Abel.ttf", "MK Latino Plain.ttf",
            "MKorsair.ttf", "Made in China.ttf", "Magnus Jockey.ttf",
            "Majetto.ttf", "Marela.ttf", "Maritime Sans.ttf",
            "Maropawi Club.ttf", "Mary Jane Larabie.ttf", "Matthan Sans Regular.ttf",
            "Mayangsari.ttf", "Meditation.ttf", "Metropolis.ttf",
            "MetropolisNF.ttf", "Micursif.ttf", "Midland Rail NF.ttf",
            "Mighty Mighty Friars.ttf", "Mignone.ttf", "Milenio-jed.ttf",
            "Milk Run.ttf", "Minya.ttf", "Mirage.ttf",
            "Mocha Regular.ttf", "Modern Curve.ttf", "Moderne Fraktur.ttf",
            "Mogambo!.ttf", "Mold Papa.ttf", "Monika.ttf",
            "Monkey.ttf", "Monograms Toolbox.ttf", "Morevil.ttf",
            "Morning Wasabi.ttf", "Morris Roman Black.ttf", "Mortis.ttf",
            "Mostlios.ttf", "Mousou Record.ttf", "Movie Letters.ttf",
            "Mr Skae.ttf", "Mudshovel.ttf", "Mura-Knockout.ttf",
            "Myndraine.ttf", "NFS Font.ttf", "Napapiiri.ttf",
            "Nashville.ttf", "Nebraska.ttf", "Nendo.ttf",
            "Neretta.ttf", "New Cicle Fina.ttf", "New Stencil TFB.ttf",
            "Newbie Serif.ttf", "Night Court.ttf", "Ninja Naruto.ttf",
            "Ninja Penguin.ttf", "Nisaba.ttf", "Nouveau IBM.ttf",
            "Nulshock Bold.ttf", "Octin College.ttf", "Old Book.ttf",
            "Old London.ttf", "Old Newspaper Types.ttf", "Old Skool Graff.ttf",
            "Old Typefaces.ttf", "Olde Chicago.ttf", "Oldstyle HPLHS.TTF",
            "OliJo Bold.ttf", "Omotenashi.ttf", "Once Upon A Time.ttf",
            "One Way.ttf", "Orchidee Medium.ttf", "Organic Fruit.ttf",
            "Origin Regular.ttf", "Osaka Sans Serif.ttf", "PAC Libertas.ttf",
            "Pacifica.ttf", "Painty Paint.ttf", "Palovsky.ttf",
            "Paragon Cleaners Medium.ttf", "Pasundan.ttf", "Patinio Graffiti.ttf",
            "Peake.ttf", "Pehuensito.ttf", "Penguin Sans.ttf",
            "PentaGram's Salemica.ttf", "Petitscript.ttf", "Pharmacy.ttf",
            "Philosopher Regular.ttf", "Phoenix Sans.ttf", "Phone Streak.ttf",
            "Phrixus.ttf", "Pinophyta.ttf", "Pirata One.ttf",
            "Pneumatics.ttf", "Pompeji Petit.ttf", "Popo.ttf",
            "Port 118.ttf", "Poseidon AOE.ttf", "Poster Font.ttf",
            "Poster Slab Caps.ttf", "Prince Dub.ttf", "Propaganda.ttf",
            "Pundit.ttf", "Pupcat.ttf", "Puritan.ttf",
            "Qhytsdakx.ttf", "Quael Gothic.ttf", "Quincaille.ttf",
            "Quirkus.ttf", "RT DIY-Tape.ttf", "Rafika.ttf",
            "Rain & Neer.ttf", "Ramsey SD.ttf", "Reactive.ttf",
            "Rebel Caps.ttf", "Rebel Redux.ttf", "Reckoner.ttf",
            "Return To Castle.ttf", "Rimouski.ttf", "Riotun.ttf",
            "Rita.ttf", "Ritalin.ttf", "Ritzy Remix.ttf",
            "River Avenue.ttf", "Robust and Husky.ttf", "Rocko.ttf",
            "Rogaton.ttf", "Rogers.ttf", "Rolling No One Extra Bold.ttf",
            "Roman Grid Caps.ttf", "Roman SD.ttf", "Romerati.ttf",
            "Roskell.ttf", "Rostock Kaligraph.ttf", "Royal.ttf",
            "Rugamika.ttf", "Rundgotisch Rauh.ttf", "Russian.ttf",
            "Rutaban.ttf", "SV Basic Manual.ttf", "Saddlebag.ttf",
            "Samurai.ttf", "Sanctuary.ttf", "Sarcophagus.ttf",
            "Satyr Passionate.ttf", "Scarface.ttf", "Sci Fly Sans.ttf",
            "Scratch.ttf", "Seattle Sans.ttf", "Secrets Stencil.ttf",
            "Sesame.ttf", "Shanghai.ttf", "Simple Life.ttf",
            "Skeleton Key.ttf", "Skinny Minnie.ttf", "Skitser Cartoon.ttf",
            "Slender.ttf", "Slice And Dice.ttf", "Smoke.ttf",
            "Snappy Service.ttf", "South Afirkas 2100.ttf", "Soy Sauce Junky.ttf",
            "Splendid Plan 9 Regular.ttf", "Sports World.ttf", "Spray.ME.ttf",
            "Square.ttf", "Stab.ttf", "Stage.ttf",
            "Stencil Export.ttf", "Still Time.ttf", "Streetvertising Medium.ttf",
            "Subway.ttf", "Sui Generis.ttf", "Summertime.ttf",
            "Sundayscript.ttf", "Super Mario 256.ttf", "Swish.ttf",
            "Tagster.ttf", "Takeout.ttf", "Teutonic.ttf",
            "Text In Gothic.ttf", "The Soul Of Vodka.ttf", "Three Sixty.ttf",
            "Timeless.ttf", "Tintoretto.ttf", "Titan One.ttf",
            "To Be Continued.ttf", "Tomipop.ttf", "Tongkonan.ttf",
            "Tork.ttf", "Tulisan Tanganku.ttf", "Turok.ttf",
            "Type Wrong.ttf", "Typewriter Oldstyle.ttf", "Typography Times Regular.ttf",
            "Typomoderno.ttf", "Tyro Sans.ttf", "Uchiyama.ttf",
            "UnZialish.ttf", "Underwood Champion.ttf", "Unik Type.ttf",
            "Unispace.ttf", "Usenet.ttf", "Vahika.ttf",
            "Vanilla Whale.ttf", "Vegas Desert.ttf", "Velvet Illusions.ttf",
            "Verve.ttf", "Vibrocentric.ttf", "Victor Hugo.ttf",
            "Victoria CAT.ttf", "Vinque.ttf", "Visionaries.ttf",
            "Vive la Rivoluzione.ttf", "Walshes.ttf", "Wartorn.ttf",
            "Washington Text.ttf", "Waterloo Relief.ttf", "Whoa!.ttf",
            "Wild Ride.ttf", "Wind Sans Serif.ttf", "Winterland.ttf",
            "Writers Original.ttf", "Xilosa.ttf", "Xipital.ttf",
            "Xirod.ttf", "Xtra.ttf", "Yaahowu.ttf",
            "Yanone Kaffeesatz Regular.ttf", "Ysgarth.ttf", "Zilluncial.TTF",
            "akaPosse.ttf", "el Diablo.ttf"
        };

        for (const auto& fontFile : knownFonts) {
            std::string fullPath = directory + fontFile;

            TTF_Font* testFont = TTF_OpenFont(fullPath.c_str(), 12);
            if (testFont) {
                TTF_CloseFont(testFont);
                // font loads ok

                std::string fontName = fontFile;
                size_t dotPos = fontName.find_last_of('.');
                if (dotPos != std::string::npos) {
                    fontName = fontName.substr(0, dotPos);
                }

                if (fontName.length() > 0 && (fontName[0] == '!' || std::isdigit(fontName[0]))) {
                    size_t firstLetter = fontName.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
                    if (firstLetter != std::string::npos) {
                        fontName = fontName.substr(firstLetter);
                    }
                }

                bool alreadyHave = false;
                for (const auto& existing : m_fontNames) {
                    if (existing == fontName) {
                        alreadyHave = true;
                        break;
                    }
                }

                if (!alreadyHave) {
                    m_availableFonts.push_back(fullPath);
                    m_fontNames.push_back(fontName);
                }
            }
        }
    } else {
        std::vector<std::string> commonFonts = {
            "arial.ttf", "times.ttf", "helvetica.ttf", "courier.ttf",
            "georgia.ttf", "verdana.ttf", "tahoma.ttf", "trebuchet.ttf"
        };

        for (const auto& fontFile : commonFonts) {
            std::string fullPath = directory + fontFile;

            TTF_Font* testFont = TTF_OpenFont(fullPath.c_str(), 12);
            if (testFont) {
                TTF_CloseFont(testFont);

                std::string fontName = fontFile;
                size_t dotPos = fontName.find_last_of('.');
                if (dotPos != std::string::npos) {
                    fontName = fontName.substr(0, dotPos);
                }

                if (!fontName.empty()) {
                    fontName[0] = std::toupper(fontName[0]);
                }

                bool alreadyHave = false;
                for (const auto& existing : m_fontNames) {
                    if (existing == fontName) {
                        alreadyHave = true;
                        break;
                    }
                }

                if (!alreadyHave) {
                    m_availableFonts.push_back(fullPath);
                    m_fontNames.push_back(fontName);
                }
            }
        }
    }
}

void TextTool::addCustomFont(const std::string& fontPath, const std::string& fontName) {
    // quick check for dupes
    for (const auto& existing : m_availableFonts) {
        if (existing == fontPath) {
            return;
        }
    }

    TTF_Font* testFont = TTF_OpenFont(fontPath.c_str(), 12);
    if (testFont) {
        TTF_CloseFont(testFont);
        m_availableFonts.push_back(fontPath);
        m_fontNames.push_back(fontName);
    }
}

void TextTool::setFontForTextBox(int index, const std::string& fontPath, const std::string& fontName) {
    if (index < 0 || index >= static_cast<int>(m_textBoxes.size())) return;

    m_textBoxes[index].fontPath = fontPath;
    m_textBoxes[index].fontName = fontName;
    m_needsUpdate = true; // FIXME: optimize this
}

void TextTool::drawTextBoxBorder(SDL_Renderer* renderer, const TextBox& textBox, bool isActive) {
    if (isActive) {
        // Double border looks bad but QA signed off (release pressure)
        SDL_SetRenderDrawColor(renderer, 0, 120, 215, 255);
        SDL_Rect outerRect = textBox.rect;
        outerRect.x -= 2;
        outerRect.y -= 2;
        outerRect.w += 4;
        outerRect.h += 4;
        SDL_RenderDrawRect(renderer, &outerRect);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
        SDL_Rect innerRect = textBox.rect;
        innerRect.x -= 1;
        innerRect.y -= 1;
        innerRect.w += 2;
        innerRect.h += 2;
        SDL_RenderDrawRect(renderer, &innerRect);
    } else {
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 128);
        SDL_RenderDrawRect(renderer, &textBox.rect);
    }
}

void HealingTool::applyHealingAt(int x, int y) {
    Canvas& canvas = GetCanvas();
    Layer* activeLayer = canvas.getActiveLayer();

    if (!activeLayer || activeLayer->isLocked()) return;

    SDL_Renderer* renderer = canvas.getRenderer();
    SDL_Texture* texture = activeLayer->getTexture();

    // Get texture dimensions
    int width, height;
    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);

    // Check bounds
    if (x < 0 || x >= width || y < 0 || y >= height) return;

    // Create surface from texture to access pixel data
    SDL_SetRenderTarget(renderer, texture);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch);

    // Healing brush algorithm: simple blur/averaging
    const int radius = m_size / 2;
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);

    // Sample surrounding area to get average color
    int sampleCount = 0;
    int redSum = 0, greenSum = 0, blueSum = 0, alphaSum = 0;

    // Get average color from surrounding pixels
    for (int dy = -radius*2; dy <= radius*2; dy++) {
        for (int dx = -radius*2; dx <= radius*2; dx++) {
            int sx = x + dx;
            int sy = y + dy;

            // Skip pixels outside the image or too close to the center
            if (sx < 0 || sx >= width || sy < 0 || sy >= height) continue;
            if (abs(dx) < radius/2 && abs(dy) < radius/2) continue;

            Uint32 pixel = pixels[sy * width + sx];
            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);

            redSum += r;
            greenSum += g;
            blueSum += b;
            alphaSum += a;
            sampleCount++;
        }
    }

    // Calculate average color
    if (sampleCount > 0) {
        Uint8 avgRed = redSum / sampleCount;
        Uint8 avgGreen = greenSum / sampleCount;
        Uint8 avgBlue = blueSum / sampleCount;
        Uint8 avgAlpha = alphaSum / sampleCount;

        // Apply healing (blend with average color)
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                int px = x + dx;
                int py = y + dy;

                if (px < 0 || px >= width || py < 0 || py >= height) continue;

                // Only affect pixels within the circular brush
                float dist = std::sqrt(dx*dx + dy*dy);
                if (dist > radius) continue;

                // Calculate blend factor based on distance from center
                float blend = 1.0f - (dist / radius);

                // Get current pixel
                Uint32 pixel = pixels[py * width + px];
                Uint8 r, g, b, a;
                SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);

                // Blend with average color
                r = static_cast<Uint8>(r * (1.0f - blend) + avgRed * blend);
                g = static_cast<Uint8>(g * (1.0f - blend) + avgGreen * blend);
                b = static_cast<Uint8>(b * (1.0f - blend) + avgBlue * blend);
                a = static_cast<Uint8>(a * (1.0f - blend) + avgAlpha * blend);

                // Update pixel
                pixels[py * width + px] = SDL_MapRGBA(surface->format, r, g, b, a);
            }
        }
    }

    // Update the texture with the modified pixel data
    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    // Set the new texture to the layer
    activeLayer->setTexture(newTexture);
    SDL_SetRenderTarget(renderer, nullptr);
}

// Clone Stamp Tool implementation
CloneStampTool::CloneStampTool() : Tool() {
    m_size = 20; // Default brush size
}

void CloneStampTool::handleMouseDown(const SDL_Event& event) {
    if (event.button.button != SDL_BUTTON_LEFT) return;

    // Check if Alt key is held for setting source point
    const Uint8* keyState = SDL_GetKeyboardState(nullptr);
    bool altHeld = (keyState[SDL_SCANCODE_LALT] || keyState[SDL_SCANCODE_RALT]);

    if (altHeld) {
        // ALT+LEFT CLICK: Set source point
        setSourcePoint(event.button.x, event.button.y);
    } else if (m_hasSourcePoint) {
        // LEFT CLICK: Start cloning
        m_isDrawing = true;
        m_isCloning = true;
        m_startPos = ImVec2(event.button.x, event.button.y);
        cloneAt(event.button.x, event.button.y);
    }
}

void CloneStampTool::handleMouseMove(const SDL_Event& event) {
    if (!m_isDrawing || !m_isCloning || !m_hasSourcePoint) return;

    m_currentPos = ImVec2(event.motion.x, event.motion.y);
    cloneAt(event.motion.x, event.motion.y);
}

void CloneStampTool::handleMouseUp(const SDL_Event& event) {
    if (event.button.button == SDL_BUTTON_LEFT) {
        m_isDrawing = false;
        m_isCloning = false;
    }
}

void CloneStampTool::render(SDL_Renderer* renderer) {
    if (m_hasSourcePoint) {
        drawSourcePreview(renderer);
    }

    // Draw brush preview at cursor
    if (m_isDrawing && m_isCloning) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        // Draw circle outline for brush preview
        int centerX = (int)m_currentPos.x;
        int centerY = (int)m_currentPos.y;
        int radius = m_size / 2;

        // Simple circle drawing using points
        for (int angle = 0; angle < 360; angle += 5) {
            float radians = angle * M_PI / 180.0f;
            int x = centerX + (int)(radius * cos(radians));
            int y = centerY + (int)(radius * sin(radians));
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }
}

void CloneStampTool::setSourcePoint(int x, int y) {
    m_sourcePoint.x = x;
    m_sourcePoint.y = y;
    m_hasSourcePoint = true;
}

void CloneStampTool::cloneAt(int x, int y) {
    Canvas& canvas = GetCanvas();
    Layer* activeLayer = canvas.getActiveLayer();
    if (!activeLayer) return;

    SDL_Renderer* renderer = canvas.getRenderer();
    if (!renderer) return;

    // Calculate offset from initial click to current position
    int offsetX = x - (int)m_startPos.x;
    int offsetY = y - (int)m_startPos.y;

    // Calculate source position (moves with the brush)
    int sourceX = m_sourcePoint.x + offsetX;
    int sourceY = m_sourcePoint.y + offsetY;

    // Get source texture (current layer for simplicity)
    SDL_Texture* sourceTexture = activeLayer->getTexture();
    if (!sourceTexture) return;

    // Set render target to active layer
    SDL_SetRenderTarget(renderer, activeLayer->getTexture());

    int brushRadius = m_size / 2;

    // Copy pixels from source to destination in circular pattern
    for (int dy = -brushRadius; dy <= brushRadius; dy++) {
        for (int dx = -brushRadius; dx <= brushRadius; dx++) {
            // Check if pixel is within circular brush
            float dist = sqrt(dx*dx + dy*dy);
            if (dist > brushRadius) continue;

            int srcX = sourceX + dx;
            int srcY = sourceY + dy;
            int dstX = x + dx;
            int dstY = y + dy;

            // Bounds checking
            if (srcX < 0 || srcX >= canvas.getWidth() || srcY < 0 || srcY >= canvas.getHeight()) continue;
            if (dstX < 0 || dstX >= canvas.getWidth() || dstY < 0 || dstY >= canvas.getHeight()) continue;

            // For simplicity, we'll use SDL_RenderCopy for small rectangles
            // In a real implementation, we'd manipulate pixels directly for better performance
            SDL_Rect srcRect = {srcX, srcY, 1, 1};
            SDL_Rect dstRect = {dstX, dstY, 1, 1};

            // Calculate opacity based on distance from center for smooth brushing
            float opacity = 1.0f - (dist / brushRadius);
            SDL_SetTextureAlphaMod(sourceTexture, (Uint8)(opacity * 255));

            // Note: This is a simplified approach. Real clone stamp would need pixel-level manipulation
            // SDL_RenderCopy(renderer, sourceTexture, &srcRect, &dstRect);
        }
    }

    // Reset alpha mod
    SDL_SetTextureAlphaMod(sourceTexture, 255);
    SDL_SetRenderTarget(renderer, nullptr);
}

void CloneStampTool::drawSourcePreview(SDL_Renderer* renderer) {
    // Draw a crosshair at the source point
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 200);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    int crossSize = 10;

    // Horizontal line
    SDL_RenderDrawLine(renderer,
                      m_sourcePoint.x - crossSize, m_sourcePoint.y,
                      m_sourcePoint.x + crossSize, m_sourcePoint.y);

    // Vertical line
    SDL_RenderDrawLine(renderer,
                      m_sourcePoint.x, m_sourcePoint.y - crossSize,
                      m_sourcePoint.x, m_sourcePoint.y + crossSize);

    // Draw circle around source point
    int radius = 8;
    for (int angle = 0; angle < 360; angle += 10) {
        float radians = angle * M_PI / 180.0f;
        int x = m_sourcePoint.x + (int)(radius * cos(radians));
        int y = m_sourcePoint.y + (int)(radius * sin(radians));
        SDL_RenderDrawPoint(renderer, x, y);
    }
}
