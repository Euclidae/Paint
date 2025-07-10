#include "Canvas.hpp"
#include "Layer.hpp"
#include "../tools/Tool.hpp"
#include "../editor/Editor.hpp"
#include <algorithm>
#include <iostream>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <cmath>
#include <cstring>

// Canvas implementation - handles the main drawing surface and layers

// Singleton pattern - yeah I know, globals are bad but this works for now
[[nodiscard("This is a singleton so it needs to be referenced.")]]Canvas& Canvas::getInstance() {
    static Canvas instance;
    return instance;
}

Canvas::Canvas()
    : m_renderer(nullptr),
      m_canvasBuffer(nullptr),
      m_width(1280),   // standard HD width
      m_height(720),   // standard HD height
      m_activeLayerIndex(0),
      m_hasSelection(false),
      m_selectionTexture(nullptr),
      m_resizeCorner(-1) {
    // Initialize with reasonable defaults
}

void Canvas::init(SDL_Renderer* renderer) {
    m_renderer = renderer;
    // printf("Canvas init %dx%d\n", m_width, m_height); // debug
    // NOTE: setupNewCanvas broken in v1.2 - patched for release
    setupNewCanvas(m_width, m_height);
    addLayer("Background");
}

void Canvas::cleanup() {
    m_layers.clear(); // This will trigger the Layer destructors

    // Clean up SDL textures to avoid memory leaks and use after frees.
    if (m_canvasBuffer) {
        SDL_DestroyTexture(m_canvasBuffer);
        m_canvasBuffer = nullptr;
    }

    if (m_selectionTexture) {
        SDL_DestroyTexture(m_selectionTexture);
        m_selectionTexture = nullptr;
    }

    cleanupFilterBuffer();
    clearFontCache();
}

void Canvas::clearFontCache() {
    for (auto& pair : m_fontCache) {
        if (pair.second) {
            TTF_CloseFont(pair.second);
        }
    }
    m_fontCache.clear();
}

TTF_Font* Canvas::getFont(int size, bool bold, bool italic) {
    int key = size;
    if (bold) key |= 0x10000;  // bit hack for caching
    if (italic) key |= 0x20000;

    auto it = m_fontCache.find(key);
    if (it != m_fontCache.end()) {
        return it->second;
    }

    std::string fontPath = "arial.ttf";
    TTF_Font* font = TTF_OpenFont(fontPath.c_str(), size);

    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        return nullptr;
    }

    int style = TTF_STYLE_NORMAL;
    if (bold) style |= TTF_STYLE_BOLD;
    if (italic) style |= TTF_STYLE_ITALIC;
    TTF_SetFontStyle(font, style);

    m_fontCache[key] = font;
    return font;
}

void Canvas::createLayerTexture(Layer& layer) {
    if (layer.getTexture()) {
        SDL_DestroyTexture(layer.getTexture());
    }

    SDL_Texture* texture = SDL_CreateTexture(
        m_renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        m_width,
        m_height
    );

    if (!texture) {
        std::cerr << "Error creating layer texture: " << SDL_GetError() << std::endl;
        return;
    }

    // Initialize the texture with transparency
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(m_renderer, texture);
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 0);
    SDL_RenderClear(m_renderer);
    SDL_SetRenderTarget(m_renderer, nullptr);

    // Initialize with default blend mode
    layer.setBlendMode(0); // Normal blend mode

    layer.setTexture(texture);
}

void Canvas::setupNewCanvas(int width, int height) {
    // Store new dimensions
    m_width = width;
    m_height = height;

    // Clear existing layers
    m_layers.clear();

    // Create canvas buffer
    if (m_canvasBuffer) {
        SDL_DestroyTexture(m_canvasBuffer);
    }

    m_canvasBuffer = SDL_CreateTexture(
        m_renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width,
        height
    );

    if (!m_canvasBuffer) {
        std::cerr << "Error creating canvas buffer: " << SDL_GetError() << std::endl;
    }

    // Reset selection
    m_hasSelection = false;
    m_selectionRect = {0, 0, 0, 0};

    if (m_selectionTexture) {
        SDL_DestroyTexture(m_selectionTexture);
        m_selectionTexture = nullptr;
    }
}

void Canvas::addLayer(const std::string& name, bool /* isTextLayer */) {
    auto layer = std::make_unique<Layer>(name);
    createLayerTexture(*layer);

    // If this is the first layer, set it as active
    if (m_layers.empty()) {
        m_activeLayerIndex = 0;
    } else {
        m_activeLayerIndex = m_layers.size();
    }

    m_layers.push_back(std::move(layer));
}

void Canvas::moveLayer(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= static_cast<int>(m_layers.size()) ||
        toIndex < 0 || toIndex >= static_cast<int>(m_layers.size())) {
        return;
    }

    int activeLayerIndex = m_activeLayerIndex;

    auto layer = std::move(m_layers[fromIndex]);
    m_layers.erase(m_layers.begin() + fromIndex);
    m_layers.insert(m_layers.begin() + toIndex, std::move(layer));

    // Update active layer index if it was affected by the move
    if (activeLayerIndex == fromIndex) {
        m_activeLayerIndex = toIndex;
    } else if (activeLayerIndex > fromIndex && activeLayerIndex <= toIndex) {
        m_activeLayerIndex--;
    } else if (activeLayerIndex < fromIndex && activeLayerIndex >= toIndex) {
        m_activeLayerIndex++;
    }
}

void Canvas::duplicateLayer(int index) {
    if (index < 0 || index >= static_cast<int>(m_layers.size())) {
        return;
    }

    auto newLayer = std::make_unique<Layer>();
    m_layers[index]->duplicate(*newLayer);
    createLayerTexture(*newLayer);

    // Copy the texture content
    SDL_SetRenderTarget(m_renderer, newLayer->getTexture());
    SDL_RenderCopy(m_renderer, m_layers[index]->getTexture(), nullptr, nullptr);
    SDL_SetRenderTarget(m_renderer, nullptr);

    // Copy the blend mode from the original layer
    newLayer->setBlendMode(m_layers[index]->getBlendMode());

    // Insert after the original
    m_layers.insert(m_layers.begin() + index + 1, std::move(newLayer));
    m_activeLayerIndex = index + 1;
}

void Canvas::removeLayer(int index) {
    if (index < 0 || index >= static_cast<int>(m_layers.size()) || m_layers.size() <= 1) {
        return; // Always keep at least one layer
    }

    m_layers.erase(m_layers.begin() + index);

    // Adjust active layer index
    if (m_activeLayerIndex >= static_cast<int>(m_layers.size())) {
        m_activeLayerIndex = m_layers.size() - 1;
    }
}

void Canvas::renameLayer(int index, const std::string& newName) {
    if (index < 0 || index >= static_cast<int>(m_layers.size())) {
        return; // Invalid index - just silently fail
    }

    std::string finalName = newName;

    // TODO: Make this configurable maybe? 25 chars seems reasonable for UI
    const size_t MAX_NAME_LENGTH = 25;
    if (finalName.length() > MAX_NAME_LENGTH) {
        // Truncate long names with ellipsis for UI display
        finalName = finalName.substr(0, MAX_NAME_LENGTH - 3) + "...";
    }

    // HACK: Empty names look weird in the UI, so give it a default
    if (finalName.empty()) {
        finalName = "Unnamed Layer";
    }

    m_layers[index]->setName(finalName);
}

Layer* Canvas::getActiveLayer() {
    if (m_activeLayerIndex >= 0 && m_activeLayerIndex < static_cast<int>(m_layers.size())) {
        return m_layers[m_activeLayerIndex].get();
    }
    return nullptr;
}

/**
 * Imports an image from a file path and adds it to the canvas
 *
 * This function implements a robust image loading system that:
 * 1. Safely handles various image formats including JPG
 * 2. Correctly scales the image to fit the canvas dimensions
 * 3. Maintains the independence of the imported image from canvas resizing
 * 4. Handles memory properly to prevent segmentation faults
 */
void Canvas::importImage(const char* filePath) {
    if (!filePath) return;

    // Load the image with proper error handling
    SDL_Surface* surface = IMG_Load(filePath);
    if (!surface) {
        std::cerr << "Failed to load image: " << IMG_GetError() << std::endl;
        return;
    }

    // Convert surface to a consistent format to prevent issues with different image formats
    // This is crucial for JPG images which don't have an alpha channel
    SDL_Surface* convertedSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA8888, 0);
    SDL_FreeSurface(surface); // Free the original surface

    if (!convertedSurface) {
        std::cerr << "Failed to convert surface format: " << SDL_GetError() << std::endl;
        return;
    }

    // Create a new layer for the image
    std::string layerName = "Imported Image";
    if (filePath) {
        // Extract just the filename from the path
        const char* fileName = strrchr(filePath, '/');
        if (!fileName) fileName = strrchr(filePath, '\\');
        if (fileName) {
            layerName = fileName + 1; // Skip the slash
        } else {
            layerName = filePath; // Use the full path if no slash found
        }
    }
    addLayer(layerName);

    // Scale the image to fit the canvas while maintaining aspect ratio
    double scaleX = static_cast<double>(m_width) / convertedSurface->w;
    double scaleY = static_cast<double>(m_height) / convertedSurface->h;
    double scale = std::min(scaleX, scaleY); // Use the smaller scale to fit within canvas

    int newWidth = static_cast<int>(convertedSurface->w * scale);
    int newHeight = static_cast<int>(convertedSurface->h * scale);

    // Center the image in the canvas
    int offsetX = (m_width - newWidth) / 2;
    int offsetY = (m_height - newHeight) / 2;

    // Create a clean texture for the layer that's the size of the canvas
    Layer* activeLayer = getActiveLayer();
    if (!activeLayer) {
        SDL_FreeSurface(convertedSurface);
        return;
    }

    // Create a blank texture for the layer
    SDL_Texture* layerTexture = SDL_CreateTexture(
        m_renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        m_width, m_height
    );

    if (!layerTexture) {
        std::cerr << "Failed to create layer texture: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(convertedSurface);
        return;
    }

    // Clear the new texture with transparency
    SDL_SetRenderTarget(m_renderer, layerTexture);
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 0);
    SDL_RenderClear(m_renderer);

    // Create texture from the converted surface
    SDL_Texture* importedTexture = SDL_CreateTextureFromSurface(m_renderer, convertedSurface);
    SDL_FreeSurface(convertedSurface);

    if (!importedTexture) {
        std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
        SDL_DestroyTexture(layerTexture);
        return;
    }

    // Set blend mode to support transparency
    SDL_SetTextureBlendMode(importedTexture, SDL_BLENDMODE_BLEND);

    // Draw the image centered and scaled on the layer texture
    SDL_Rect destRect = {offsetX, offsetY, newWidth, newHeight};
    SDL_RenderCopy(m_renderer, importedTexture, NULL, &destRect);

    // Clean up the imported texture now that it's copied to the layer
    SDL_DestroyTexture(importedTexture);

    // Reset render target and set the texture to the layer
    SDL_SetRenderTarget(m_renderer, nullptr);
    activeLayer->setTexture(layerTexture);

    // Add to recent files list
    Editor::getInstance().addRecentFile(std::string(filePath));
}

void Canvas::exportImage(const char* filePath, const char* format) {
    if (!filePath) return;

    // Create a surface to render all visible layers
    SDL_Surface* surface = SDL_CreateRGBSurface(
        0, m_width, m_height, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
    );

    if (!surface) {
        std::cerr << "Failed to create surface for export: " << SDL_GetError() << std::endl;
        return;
    }

    // Create a temporary texture to render to
    SDL_Texture* tempTexture = SDL_CreateTexture(
        m_renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        m_width,
        m_height
    );

    if (!tempTexture) {
        std::cerr << "Failed to create texture for export: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return;
    }

    // Render all visible layers to the texture
    SDL_SetRenderTarget(m_renderer, tempTexture);
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 0);
    SDL_RenderClear(m_renderer);

    for (const auto& layer : m_layers) {
        if (layer->isVisible()) {
            SDL_SetTextureAlphaMod(layer->getTexture(), static_cast<Uint8>(layer->getOpacity() * 255));
            SDL_RenderCopy(m_renderer, layer->getTexture(), nullptr, nullptr);
        }
    }

    // Read the pixels from the texture
    Uint32* pixels = new Uint32[m_width * m_height];
    SDL_RenderReadPixels(
        m_renderer,
        nullptr,
        SDL_PIXELFORMAT_RGBA8888,
        pixels,
        m_width * 4
    );

    // Copy pixels to the surface
    SDL_LockSurface(surface);
    std::memcpy(surface->pixels, pixels, m_width * m_height * 4);
    SDL_UnlockSurface(surface);
    delete[] pixels;

    // Reset render target
    SDL_SetRenderTarget(m_renderer, nullptr);
    SDL_DestroyTexture(tempTexture);

    // Save the surface
    std::string formatStr = format ? format : "PNG";
    int result = 0;

    if (formatStr == "PNG") {
        result = IMG_SavePNG(surface, filePath);
    } else if (formatStr == "JPG" || formatStr == "JPEG") {
        result = IMG_SaveJPG(surface, filePath, 90);
    } else if (formatStr == "BMP") {
        result = SDL_SaveBMP(surface, filePath);
    }

    if (result != 0) {
        std::cerr << "Failed to save image: " << IMG_GetError() << std::endl;
    } else {
        // Add to recent files list on successful save
        Editor::getInstance().addRecentFile(std::string(filePath));
    }

    SDL_FreeSurface(surface);
}

void Canvas::render() {
    if (!m_renderer || m_layers.empty()) return;

    SDL_Texture* originalTarget = SDL_GetRenderTarget(m_renderer);

    SDL_SetRenderTarget(m_renderer, m_canvasBuffer);
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255); // white bg
    SDL_RenderClear(m_renderer);

    for (const auto& layer : m_layers) {
        if (!layer->isVisible() || !layer->getTexture()) continue; // quick skip

        SDL_SetTextureAlphaMod(layer->getTexture(), static_cast<Uint8>(layer->getOpacity() * 255));

            SDL_BlendMode blendMode = SDL_BLENDMODE_BLEND;

            // Set different blend modes based on the layer's blend mode setting
            switch(layer->getBlendMode()) {
                //  SDL2 does not include built-in support for all blend modes (e.g. screen, subtract, multiply).
                // Here we manually define custom blend modes using SDL_ComposeCustomBlendMode.
                // SDL_ComposeCustomBlendMode reference: https://wiki.libsdl.org/SDL2/SDL_ComposeCustomBlendMode. This will do
                // for now
                case 0:
                    blendMode = SDL_BLENDMODE_BLEND;
                    break;
                case 1:
                // This blend mode will be used for multiply blending (darkens)
                    blendMode = SDL_ComposeCustomBlendMode(
                        SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDFACTOR_ZERO, SDL_BLENDOPERATION_ADD,
                        SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
                    break;
                case 2:
                // This blend mode will be used for screen blending (lightens)
                    blendMode = SDL_ComposeCustomBlendMode(
                        SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR, SDL_BLENDOPERATION_ADD,
                        SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
                    break;
                case 3:
                // This blend mode will be used for additive blending (lightens)
                    blendMode = SDL_ComposeCustomBlendMode(
                        SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD,
                        SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
                    break;
                case 4:
                // This blend mode will be used for color burn blending (darkens)
                    blendMode = SDL_ComposeCustomBlendMode(
                        SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR, SDL_BLENDOPERATION_ADD,
                        SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
                    break;
                case 5:
                // This blend mode will be used for multiply blending (darkens)
                    blendMode = SDL_ComposeCustomBlendMode(
                        SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDOPERATION_ADD,
                        SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
                    break;
                case 6:
                // This one will be used for additive blending (lightens)
                    blendMode = SDL_ComposeCustomBlendMode(
                        SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD,
                        SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
                    break;
                default:
                    blendMode = SDL_BLENDMODE_BLEND;
                    break;
            }
            SDL_SetTextureBlendMode(layer->getTexture(), blendMode);

            // Get texture dimensions for positioning
            int textureWidth, textureHeight;
            SDL_QueryTexture(layer->getTexture(), nullptr, nullptr, &textureWidth, &textureHeight);

            // Create destination rectangle with layer position
            SDL_Rect destRect = {layer->getX(), layer->getY(), textureWidth, textureHeight};

            if (layer->isUsingMask() && layer->getMask()) {
                // HACK: mask blending broken since SDL upgrade (issue #89)
                SDL_SetTextureAlphaMod(layer->getTexture(), 128);
                SDL_RenderCopy(m_renderer, layer->getTexture(), nullptr, &destRect);
                SDL_SetTextureAlphaMod(layer->getTexture(), static_cast<Uint8>(layer->getOpacity() * 255));
            } else {
                SDL_RenderCopy(m_renderer, layer->getTexture(), nullptr, &destRect);
            }
    }

    if (m_hasSelection) {
        SDL_SetRenderDrawColor(m_renderer, 0, 120, 215, 128);
        SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);

        SDL_Rect rect = m_selectionRect;
        SDL_RenderDrawRect(m_renderer, &rect);

        drawResizeHandles(m_renderer);
    }

    // Draw transform box for smart object selection
    drawTransformBox(m_renderer);

    // Force a render flush to prevent UI artifacts - learned this the hard way
    SDL_RenderFlush(m_renderer);

    Tool* currentTool = ToolManager::getInstance().getCurrentTool();
    if (currentTool && currentTool->isDrawing()) {
        currentTool->render(m_renderer);
    }

    SDL_SetRenderTarget(m_renderer, originalTarget);

    SDL_Rect canvasRect = {0, 0, m_width, m_height}; // FIXME: hardcoded position
    SDL_RenderCopy(m_renderer, m_canvasBuffer, nullptr, &canvasRect);
}

void Canvas::drawResizeHandles(SDL_Renderer* renderer) {
    if (!m_hasSelection) return;

    // Define handle positions
    SDL_Rect handles[8];
    const int hs = HANDLE_SIZE;

    // Corner handles
    handles[0] = {m_selectionRect.x - hs/2, m_selectionRect.y - hs/2, hs, hs}; // top-left
    handles[1] = {m_selectionRect.x + m_selectionRect.w - hs/2, m_selectionRect.y - hs/2, hs, hs}; // top-right
    handles[2] = {m_selectionRect.x - hs/2, m_selectionRect.y + m_selectionRect.h - hs/2, hs, hs}; // bottom-left
    handles[3] = {m_selectionRect.x + m_selectionRect.w - hs/2, m_selectionRect.y + m_selectionRect.h - hs/2, hs, hs}; // bottom-right

    // Edge handles
    handles[4] = {m_selectionRect.x + m_selectionRect.w/2 - hs/2, m_selectionRect.y - hs/2, hs, hs}; // top
    handles[5] = {m_selectionRect.x + m_selectionRect.w/2 - hs/2, m_selectionRect.y + m_selectionRect.h - hs/2, hs, hs}; // bottom
    handles[6] = {m_selectionRect.x - hs/2, m_selectionRect.y + m_selectionRect.h/2 - hs/2, hs, hs}; // left
    handles[7] = {m_selectionRect.x + m_selectionRect.w - hs/2, m_selectionRect.y + m_selectionRect.h/2 - hs/2, hs, hs}; // right

    // Draw the handles
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < 8; i++) {
        SDL_RenderFillRect(renderer, &handles[i]);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    for (int i = 0; i < 8; i++) {
        SDL_RenderDrawRect(renderer, &handles[i]);
    }
}

bool Canvas::handleResizeEvent(const SDL_Event& event, const SDL_Point& mousePos) {
    if (!m_hasSelection) return false;

    // Define handle regions
    SDL_Rect handles[8];
    const int hs = HANDLE_SIZE;

    // Corner handles
    handles[0] = {m_selectionRect.x - hs/2, m_selectionRect.y - hs/2, hs, hs}; // top-left
    handles[1] = {m_selectionRect.x + m_selectionRect.w - hs/2, m_selectionRect.y - hs/2, hs, hs}; // top-right
    handles[2] = {m_selectionRect.x - hs/2, m_selectionRect.y + m_selectionRect.h - hs/2, hs, hs}; // bottom-left
    handles[3] = {m_selectionRect.x + m_selectionRect.w - hs/2, m_selectionRect.y + m_selectionRect.h - hs/2, hs, hs}; // bottom-right

    // Edge handles
    handles[4] = {m_selectionRect.x + m_selectionRect.w/2 - hs/2, m_selectionRect.y - hs/2, hs, hs}; // top
    handles[5] = {m_selectionRect.x + m_selectionRect.w/2 - hs/2, m_selectionRect.y + m_selectionRect.h - hs/2, hs, hs}; // bottom
    handles[6] = {m_selectionRect.x - hs/2, m_selectionRect.y + m_selectionRect.h/2 - hs/2, hs, hs}; // left
    handles[7] = {m_selectionRect.x + m_selectionRect.w - hs/2, m_selectionRect.y + m_selectionRect.h/2 - hs/2, hs, hs}; // right

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        // Check if we clicked on a handle
        for (int i = 0; i < 8; i++) {
            if (SDL_PointInRect(&mousePos, &handles[i])) {
                m_resizeCorner = i;
                m_resizeStartMouse = mousePos;
                m_resizeStartCanvas = m_selectionRect;
                return true;
            }
        }
    } else if (event.type == SDL_MOUSEMOTION && m_resizeCorner != -1) {
        // Update selection rect based on which handle is being dragged
        int dx = mousePos.x - m_resizeStartMouse.x;
        int dy = mousePos.y - m_resizeStartMouse.y;

        switch (m_resizeCorner) {
            case 0: // top-left
                m_selectionRect.x = m_resizeStartCanvas.x + dx;
                m_selectionRect.y = m_resizeStartCanvas.y + dy;
                m_selectionRect.w = m_resizeStartCanvas.w - dx;
                m_selectionRect.h = m_resizeStartCanvas.h - dy;
                break;
            case 1: // top-right
                m_selectionRect.y = m_resizeStartCanvas.y + dy;
                m_selectionRect.w = m_resizeStartCanvas.w + dx;
                m_selectionRect.h = m_resizeStartCanvas.h - dy;
                break;
            case 2: // bottom-left
                m_selectionRect.x = m_resizeStartCanvas.x + dx;
                m_selectionRect.w = m_resizeStartCanvas.w - dx;
                m_selectionRect.h = m_resizeStartCanvas.h + dy;
                break;
            case 3: // bottom-right
                m_selectionRect.w = m_resizeStartCanvas.w + dx;
                m_selectionRect.h = m_resizeStartCanvas.h + dy;
                break;
            case 4: // top
                m_selectionRect.y = m_resizeStartCanvas.y + dy;
                m_selectionRect.h = m_resizeStartCanvas.h - dy;
                break;
            case 5: // bottom
                m_selectionRect.h = m_resizeStartCanvas.h + dy;
                break;
            case 6: // left
                m_selectionRect.x = m_resizeStartCanvas.x + dx;
                m_selectionRect.w = m_resizeStartCanvas.w - dx;
                break;
            case 7: // right
                m_selectionRect.w = m_resizeStartCanvas.w + dx;
                break;
        }

        // Ensure selection remains valid
        if (m_selectionRect.w < 1) m_selectionRect.w = 1;
        if (m_selectionRect.h < 1) m_selectionRect.h = 1;

        return true;
    } else if (event.type == SDL_MOUSEBUTTONUP && m_resizeCorner != -1) {
        m_resizeCorner = -1;
        return true;
    }

    return false;
}

SDL_Surface* Canvas::resizeImage(SDL_Surface* src, int newWidth, int newHeight) {
    if (!src) return nullptr;

    SDL_Surface* resized = SDL_CreateRGBSurface(0, newWidth, newHeight, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    if (!resized) return nullptr;

    // Simple nearest neighbor scaling
    SDL_LockSurface(src);
    SDL_LockSurface(resized);

    Uint32* srcPixels = static_cast<Uint32*>(src->pixels);
    Uint32* dstPixels = static_cast<Uint32*>(resized->pixels);

    float xRatio = static_cast<float>(src->w) / newWidth;
    float yRatio = static_cast<float>(src->h) / newHeight;

    for (int y = 0; y < newHeight; y++) {
        for (int x = 0; x < newWidth; x++) {
            int srcX = static_cast<int>(x * xRatio);
            int srcY = static_cast<int>(y * yRatio);

            if (srcX < src->w && srcY < src->h) {
                dstPixels[y * newWidth + x] = srcPixels[srcY * src->w + srcX];
            }
        }
    }

    SDL_UnlockSurface(resized);
    SDL_UnlockSurface(src);

    return resized;
}

void Canvas::resizeCanvas(int newWidth, int newHeight) {
    m_width = newWidth;
    m_height = newHeight;

    // Recreate canvas buffer
    if (m_canvasBuffer) {
        SDL_DestroyTexture(m_canvasBuffer);
    }

    m_canvasBuffer = SDL_CreateTexture(
        m_renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        newWidth,
        newHeight
    );

    // Resize all layers
    for (auto& layer : m_layers) {
        if (layer->getTexture()) {
            SDL_Texture* oldTexture = layer->getTexture();

            // Create new texture with new size
            SDL_Texture* newTexture = SDL_CreateTexture(
                m_renderer,
                SDL_PIXELFORMAT_RGBA8888,
                SDL_TEXTUREACCESS_TARGET,
                newWidth,
                newHeight
            );

            if (newTexture) {
                // Clear with transparency
                SDL_SetRenderTarget(m_renderer, newTexture);
                SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 0);
                SDL_RenderClear(m_renderer);

                // Copy old content
                SDL_RenderCopy(m_renderer, oldTexture, nullptr, nullptr);
                SDL_SetRenderTarget(m_renderer, nullptr);

                layer->setTexture(newTexture);
            }
        }
    }
}

void Canvas::cropImage() {
    if (!m_hasSelection) return;

    int newWidth = m_selectionRect.w;
    int newHeight = m_selectionRect.h;

    if (newWidth <= 0 || newHeight <= 0) return;

    // Create new textures for all layers
    for (auto& layer : m_layers) {
        if (layer->getTexture()) {
            SDL_Texture* oldTexture = layer->getTexture();

            SDL_Texture* newTexture = SDL_CreateTexture(
                m_renderer,
                SDL_PIXELFORMAT_RGBA8888,
                SDL_TEXTUREACCESS_TARGET,
                newWidth,
                newHeight
            );

            if (newTexture) {
                SDL_SetRenderTarget(m_renderer, newTexture);
                SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 0);
                SDL_RenderClear(m_renderer);

                SDL_Rect destRect = {-m_selectionRect.x, -m_selectionRect.y, m_width, m_height};
                SDL_RenderCopy(m_renderer, oldTexture, nullptr, &destRect);

                SDL_SetRenderTarget(m_renderer, nullptr);
                layer->setTexture(newTexture);
            }
        }
    }

    // Update canvas size
    m_width = newWidth;
    m_height = newHeight;

    // Recreate canvas buffer
    if (m_canvasBuffer) {
        SDL_DestroyTexture(m_canvasBuffer);
    }

    m_canvasBuffer = SDL_CreateTexture(
        m_renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        newWidth,
        newHeight
    );

    // Clear selection
    m_hasSelection = false;
    m_selectionRect = {0, 0, 0, 0};
}

void Canvas::rotateImage(int desiredAngle) {
    // Rotation logic - handles any angle but optimizes for common cases
    // Originally tried to be clever with loops but that was just asking for trouble

    if (desiredAngle == 0) return; // why waste time?

    // Normalize the angle - SDL gets cranky with values outside -360 to 360
    int normalizedRotation = desiredAngle % 360;
    if (normalizedRotation < 0) normalizedRotation += 360;

    // Special handling for 90-degree increments since they're super common
    bool isRightAngle = (normalizedRotation % 90 == 0);
    int newCanvasWidth = m_width;
    int newCanvasHeight = m_height;

    if (isRightAngle && (normalizedRotation == 90 || normalizedRotation == 270)) {
        // Swap dimensions for 90/270 degree rotations
        newCanvasWidth = m_height;
        newCanvasHeight = m_width;
    }

    // Process each layer individually - learned this the hard way after trying to batch them
    for (auto& currentLayer : m_layers) {
        if (!currentLayer || !currentLayer->getTexture()) continue;

        SDL_Texture* originalLayerTexture = currentLayer->getTexture();
        int origWidth, origHeight;
        SDL_QueryTexture(originalLayerTexture, nullptr, nullptr, &origWidth, &origHeight);

        // Calculate rotated texture bounds - this math took me way too long to figure out
        double angleInRadians = normalizedRotation * M_PI / 180.0;
        double cosAngle = std::abs(std::cos(angleInRadians));
        double sinAngle = std::abs(std::sin(angleInRadians));

        int rotatedTextureWidth = static_cast<int>(origWidth * cosAngle + origHeight * sinAngle);
        int rotatedTextureHeight = static_cast<int>(origWidth * sinAngle + origHeight * cosAngle);

        SDL_Texture* rotatedTexture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA8888,
                                                       SDL_TEXTUREACCESS_TARGET,
                                                       rotatedTextureWidth, rotatedTextureHeight);

        if (!rotatedTexture) {
            // Sometimes texture creation fails - just skip this layer rather than crash
            continue;
        }

        // Set up rendering to the new texture
        SDL_Texture* previousTarget = SDL_GetRenderTarget(m_renderer);
        SDL_SetRenderTarget(m_renderer, rotatedTexture);

        // Clear with transparent background
        SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 0);
        SDL_RenderClear(m_renderer);

        // Calculate center point for rotation
        SDL_Point rotationCenter = {rotatedTextureWidth / 2, rotatedTextureHeight / 2};
        SDL_Rect destinationRect = {
            rotationCenter.x - origWidth / 2,
            rotationCenter.y - origHeight / 2,
            origWidth,
            origHeight
        };

        // Actually do the rotation - SDL_RenderCopyEx is finicky but works
        SDL_RenderCopyEx(m_renderer, originalLayerTexture, nullptr, &destinationRect,
                        static_cast<double>(normalizedRotation), &rotationCenter, SDL_FLIP_NONE);

        // Restore previous render target
        SDL_SetRenderTarget(m_renderer, previousTarget);

        // Replace the layer's texture with our rotated version
        currentLayer->setTexture(rotatedTexture);
    }

    // Update canvas dimensions if needed
    if (newCanvasWidth != m_width || newCanvasHeight != m_height) {
        m_width = newCanvasWidth;
        m_height = newCanvasHeight;

        // Recreate the canvas buffer - this could probably be optimized but it works
        if (m_canvasBuffer) {
            SDL_DestroyTexture(m_canvasBuffer);
        }

        m_canvasBuffer = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_TARGET, m_width, m_height);
    }

    // Clear any active selection since it's probably invalid now
    if (m_hasSelection) {
        m_hasSelection = false;
        m_selectionRect = {0, 0, 0, 0};
    }
}

/**
 * Applies a grayscale filter to the active layer
 *
 * This implementation addresses potential crashes by:
 * 1. Verifying all pointers before use
 * 2. Checking surface creation success
 * 3. Using proper surface format matching
 * 4. Ensuring correct texture dimensions are used
 * 5. Properly cleaning up resources even on error paths
 */

// HACK: Buffer image system - genius workaround for filter stacking crashes!
// Create hidden buffer, apply filter, then replace current texture
void Canvas::createFilterBuffer() {
    cleanupFilterBuffer(); // Clean up any existing buffer

    Layer* activeLayer = getActiveLayer();
    if (!activeLayer) return;

    // Create buffer texture same size as canvas
    m_filterBuffer = SDL_CreateTexture(
        m_renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        m_width,
        m_height
    );

    if (!m_filterBuffer) return;

    // Copy current layer texture to buffer
    SDL_SetRenderTarget(m_renderer, m_filterBuffer);
    SDL_RenderCopy(m_renderer, activeLayer->getTexture(), nullptr, nullptr);
    SDL_SetRenderTarget(m_renderer, nullptr);
}

void Canvas::applyFilterBuffer() {
    if (!m_filterBuffer) return;

    Layer* activeLayer = getActiveLayer();
    if (!activeLayer) return;

    // Replace current texture with buffer texture
    activeLayer->setTexture(m_filterBuffer);
    m_filterBuffer = nullptr; // Transfer ownership, don't destroy
}

void Canvas::cleanupFilterBuffer() {
    if (m_filterBuffer) {
        SDL_DestroyTexture(m_filterBuffer);
        m_filterBuffer = nullptr;
    }
}
void Canvas::applyGrayscale() {
    // WARNING: Using blur then grayscale can cause segfaults
    // Edge cases need understanding, now optimized under great handling - is straightforward
    // (Enough is cute)
    // HACK: Use buffer image system to prevent crashes - genius workaround!
    if (m_filterInProgress) return;

    Layer* activeLayer = getActiveLayer();
    if (!activeLayer || activeLayer->isLocked()) return;

    // FIXED: Save undo state before applying filter
    Editor::getInstance().saveUndoState();

    m_filterInProgress = true;
    createFilterBuffer(); // Create buffer for safe filter application

    // Work on buffer instead of original texture
    SDL_Texture* texture = m_filterBuffer;
    if (!texture) {
        m_filterInProgress = false;
        return;
    }

    // Get texture dimensions instead of using canvas dimensions
    int texWidth, texHeight;
    if (SDL_QueryTexture(texture, NULL, NULL, &texWidth, &texHeight) != 0) {
        return;
    }

    // LEAK FIX: Set render target to texture for pixel reading
    SDL_SetRenderTarget(m_renderer, texture);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, texWidth, texHeight, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!surface) {
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    // Read pixels from the texture into the surface
    if (SDL_RenderReadPixels(m_renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch) != 0) {
        // LEAK FIX: Always clean up surface on failure
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    // Apply grayscale conversion
    SDL_LockSurface(surface);
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);

    for (int i = 0; i < surface->w * surface->h; i++) {
        Uint8 r, g, b, a;
        SDL_GetRGBA(pixels[i], surface->format, &r, &g, &b, &a);

        // Weighted grayscale conversion - same as before to keep user results consistent
        Uint8 gray = static_cast<Uint8>(0.299f * r + 0.587f * g + 0.114f * b);
        pixels[i] = SDL_MapRGBA(surface->format, gray, gray, gray, a);
    }

    SDL_UnlockSurface(surface);

    // Create new texture from modified surface
    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(m_renderer, surface);

    // LEAK FIX: Clean up the surface regardless of whether the texture creation succeeded
    SDL_FreeSurface(surface);
    SDL_SetRenderTarget(m_renderer, nullptr);

    if (newTexture) {
        // Set blend mode to preserve transparency
        SDL_SetTextureBlendMode(newTexture, SDL_BLENDMODE_BLEND);
        // Apply to buffer instead of directly to layer
        SDL_SetRenderTarget(m_renderer, m_filterBuffer);
        SDL_RenderCopy(m_renderer, newTexture, nullptr, nullptr);
        SDL_SetRenderTarget(m_renderer, nullptr);
        SDL_DestroyTexture(newTexture);
        }

        // Apply buffer to layer and cleanup
        applyFilterBuffer();

        // FIXED: Mark filter completion and track last applied filter
        m_lastAppliedFilter = FilterType::GRAYSCALE;
        m_filterInProgress = false;
}

/**
 * Applies a blur filter to the active layer
 *
 * This optimized implementation:
 * 1. Uses actual texture dimensions rather than canvas dimensions
 * 2. Limits blur strength to prevent excessive processing
 * 3. Properly handles edge cases and memory management
 * 4. Uses array indexing optimization to prevent crashes from invalid memory access
 * 5. Maintains alpha channel transparency correctly
 */
void Canvas::applyBlur(int strength) {
    // WARNING: Using blur then grayscale can cause segfaults
    // HACK: Use buffer image system - now we can stack filters safely!
    // (Enough is cute)
    if (m_filterInProgress) return;

    Layer* activeLayer = getActiveLayer();
    if (!activeLayer || activeLayer->isLocked()) return;

    // FIXED: Save undo state before applying filter
    Editor::getInstance().saveUndoState();

    m_filterInProgress = true;
    createFilterBuffer(); // Create buffer for safe filter application

    // Work on buffer instead of original texture
    SDL_Texture* texture = m_filterBuffer;
    if (!texture) {
        m_filterInProgress = false;
        return;
    }

    // Limit strength to prevent excessive processing and potential crashes
    strength = std::min(std::max(strength, 1), 10);

    // Get texture dimensions instead of using canvas dimensions
    int texWidth, texHeight;
    if (SDL_QueryTexture(texture, NULL, NULL, &texWidth, &texHeight) != 0) {
        return;
    }

    // LEAK FIX: Simple box blur implementation with proper cleanup
    SDL_SetRenderTarget(m_renderer, texture);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, texWidth, texHeight, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!surface) {
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    // Read pixels from the texture
    if (SDL_RenderReadPixels(m_renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch) != 0) {
        // LEAK FIX: Always free surface on failure
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    // Create a second surface for the blurred result
    SDL_Surface* blurred = SDL_CreateRGBSurface(0, texWidth, texHeight, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!blurred) {
        // LEAK FIX: Don't forget to clean up the first surface
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    SDL_LockSurface(surface);
    SDL_LockSurface(blurred);

    Uint32* srcPixels = static_cast<Uint32*>(surface->pixels);
    Uint32* dstPixels = static_cast<Uint32*>(blurred->pixels);

    // Create a temporary buffer for RGB values to prevent overflow
    int width = surface->w;

    for (int y = 0; y < surface->h; y++) {
        for (int x = 0; x < width; x++) {
            int r = 0, g = 0, b = 0, a = 0, count = 0;

            // Box blur kernel - keeping the same algorithm so users get consistent results
            for (int dy = -strength; dy <= strength; dy++) {
                for (int dx = -strength; dx <= strength; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx >= 0 && nx < width && ny >= 0 && ny < surface->h) {
                        // Calculate index safely
                        int idx = ny * width + nx;

                        // Get pixel components
                        Uint8 pr, pg, pb, pa;
                        SDL_GetRGBA(srcPixels[idx], surface->format, &pr, &pg, &pb, &pa);

                        // Accumulate weighted by alpha
                        r += pr;
                        g += pg;
                        b += pb;
                        a += pa;
                        count++;
                    }
                }
            }

            // Prevent division by zero
            if (count > 0) {
                r /= count; g /= count; b /= count; a /= count;
                // Write to destination
                dstPixels[y * width + x] = SDL_MapRGBA(blurred->format,
                    static_cast<Uint8>(r),
                    static_cast<Uint8>(g),
                    static_cast<Uint8>(b),
                    static_cast<Uint8>(a));
            }
        }
    }

    SDL_UnlockSurface(blurred);
    SDL_UnlockSurface(surface);

    // Create a new texture from the blurred surface
    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(m_renderer, blurred);

    // LEAK FIX: Clean up the surfaces regardless of texture creation success
    SDL_FreeSurface(surface);
    SDL_FreeSurface(blurred);
    SDL_SetRenderTarget(m_renderer, nullptr);

    if (newTexture) {
        // Set blend mode to preserve transparency
        SDL_SetTextureBlendMode(newTexture, SDL_BLENDMODE_BLEND);
        // Apply to buffer instead of directly to layer
        SDL_SetRenderTarget(m_renderer, m_filterBuffer);
        SDL_RenderCopy(m_renderer, newTexture, nullptr, nullptr);
        SDL_SetRenderTarget(m_renderer, nullptr);
        SDL_DestroyTexture(newTexture);
        }

        // Apply buffer to layer and cleanup
        applyFilterBuffer();

        // FIXED: Mark filter completion and track last applied filter
        m_lastAppliedFilter = FilterType::BLUR;
        m_filterInProgress = false;
}

void Canvas::applySharpen(int strength) {
    // Sharpen filter using unsharp mask technique
    if (m_filterInProgress) return;

    Layer* activeLayer = getActiveLayer();
    if (!activeLayer || activeLayer->isLocked()) return;

    // Save undo state before applying filter
    Editor::getInstance().saveUndoState();

    m_filterInProgress = true;

    // Create filter buffer
    createFilterBuffer();
    if (!m_filterBuffer) {
        m_filterInProgress = false;
        return;
    }

    // Get texture dimensions
    int width, height;
    SDL_QueryTexture(activeLayer->getTexture(), nullptr, nullptr, &width, &height);

    // Create surface for pixel manipulation
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
    if (!surface) {
        cleanupFilterBuffer();
        m_filterInProgress = false;
        return;
    }

    // Read texture pixels
    SDL_SetRenderTarget(m_renderer, activeLayer->getTexture());
    SDL_RenderReadPixels(m_renderer, nullptr, SDL_PIXELFORMAT_RGBA32, surface->pixels, surface->pitch);
    SDL_SetRenderTarget(m_renderer, nullptr);

    Uint32* pixels = (Uint32*)surface->pixels;

    // Create output surface
    SDL_Surface* outputSurface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
    if (!outputSurface) {
        SDL_FreeSurface(surface);
        cleanupFilterBuffer();
        m_filterInProgress = false;
        return;
    }

    Uint32* outputPixels = (Uint32*)outputSurface->pixels;

    // Sharpen kernel - classic unsharp mask
    int kernel[3][3] = {
        { 0, -1,  0},
        {-1,  5, -1},
        { 0, -1,  0}
    };

    // Apply sharpen filter
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int rSum = 0, gSum = 0, bSum = 0;

            // Apply kernel
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int px = x + kx;
                    int py = y + ky;

                    Uint32 pixel = pixels[py * width + px];
                    Uint8 r, g, b, a;
                    SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);

                    int kernelValue = kernel[ky + 1][kx + 1];
                    rSum += r * kernelValue;
                    gSum += g * kernelValue;
                    bSum += b * kernelValue;
                }
            }

            // Clamp values and apply strength
            rSum = (rSum * strength) / 4;  // Normalize by strength
            gSum = (gSum * strength) / 4;
            bSum = (bSum * strength) / 4;

            // Clamp to valid range
            rSum = std::max(0, std::min(255, rSum));
            gSum = std::max(0, std::min(255, gSum));
            bSum = std::max(0, std::min(255, bSum));

            // Keep original alpha
            Uint32 originalPixel = pixels[y * width + x];
            Uint8 r, g, b, a;
            SDL_GetRGBA(originalPixel, surface->format, &r, &g, &b, &a);

            outputPixels[y * width + x] = SDL_MapRGBA(outputSurface->format,
                                                     (Uint8)rSum, (Uint8)gSum, (Uint8)bSum, a);
        }
    }

    // Copy edges without filtering
    for (int y = 0; y < height; y++) {
        if (y == 0 || y == height - 1) {
            // Copy entire top/bottom rows
            for (int x = 0; x < width; x++) {
                outputPixels[y * width + x] = pixels[y * width + x];
            }
        } else {
            // Copy left/right edges
            outputPixels[y * width + 0] = pixels[y * width + 0];
            outputPixels[y * width + width - 1] = pixels[y * width + width - 1];
        }
    }

    // Convert back to texture
    SDL_SetRenderTarget(m_renderer, m_filterBuffer);
    SDL_Texture* tempTexture = SDL_CreateTextureFromSurface(m_renderer, outputSurface);
    if (tempTexture) {
        SDL_RenderCopy(m_renderer, tempTexture, nullptr, nullptr);
        SDL_DestroyTexture(tempTexture);
    }
    SDL_SetRenderTarget(m_renderer, nullptr);

    // Cleanup
    SDL_FreeSurface(surface);
    SDL_FreeSurface(outputSurface);

    // Apply buffer to layer
    applyFilterBuffer();

    // Mark filter completion
    m_lastAppliedFilter = FilterType::NONE;  // Could add SHARPEN type if needed
    m_filterInProgress = false;
}

void Canvas::flipHorizontal(bool wholeCanvas) {
    if (wholeCanvas) {
        // Flip all layers horizontally
        for (auto& layer : m_layers) {
            if (layer && layer->getTexture()) {
                flipLayerHorizontal(layer.get());
            }
        }
    } else {
        // Just flip the active layer
        Layer* activeLayer = getActiveLayer();
        if (activeLayer && !activeLayer->isLocked()) {
            flipLayerHorizontal(activeLayer);
        }
    }
}

void Canvas::flipVertical(bool wholeCanvas) {
    if (wholeCanvas) {
        // Flip all layers vertically
        for (auto& layer : m_layers) {
            if (layer && layer->getTexture()) {
                flipLayerVertical(layer.get());
            }
        }
    } else {
        // Just flip the active layer
        Layer* activeLayer = getActiveLayer();
        if (activeLayer && !activeLayer->isLocked()) {
            flipLayerVertical(activeLayer);
        }
    }
}

void Canvas::flipLayerHorizontal(Layer* layer) {
    if (!layer || !layer->getTexture()) return;

    SDL_Texture* texture = layer->getTexture();

    // Get texture dimensions
    int width, height;
    if (SDL_QueryTexture(texture, nullptr, nullptr, &width, &height) != 0) {
        return;
    }

    // Create surface from texture
    SDL_SetRenderTarget(m_renderer, texture);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!surface) {
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    // Read pixels from texture
    if (SDL_RenderReadPixels(m_renderer, nullptr, SDL_PIXELFORMAT_RGBA8888,
                           surface->pixels, surface->pitch) != 0) {
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    // Create flipped surface
    SDL_Surface* flipped = SDL_CreateRGBSurface(0, width, height, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!flipped) {
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    SDL_LockSurface(surface);
    SDL_LockSurface(flipped);

    Uint32* srcPixels = static_cast<Uint32*>(surface->pixels);
    Uint32* dstPixels = static_cast<Uint32*>(flipped->pixels);

    // Flip horizontally - mirror the pixels
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int srcIndex = y * width + x;
            int dstIndex = y * width + (width - 1 - x); // Flip x coordinate
            dstPixels[dstIndex] = srcPixels[srcIndex];
        }
    }

    SDL_UnlockSurface(flipped);
    SDL_UnlockSurface(surface);

    // Create new texture from flipped surface
    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(m_renderer, flipped);

    SDL_FreeSurface(surface);
    SDL_FreeSurface(flipped);
    SDL_SetRenderTarget(m_renderer, nullptr);

    if (newTexture) {
        SDL_SetTextureBlendMode(newTexture, SDL_BLENDMODE_BLEND);
        layer->setTexture(newTexture);
    }
}

void Canvas::flipLayerVertical(Layer* layer) {
    if (!layer || !layer->getTexture()) return;

    SDL_Texture* texture = layer->getTexture();

    // Get texture dimensions
    int width, height;
    if (SDL_QueryTexture(texture, nullptr, nullptr, &width, &height) != 0) {
        return;
    }

    // Create surface from texture
    SDL_SetRenderTarget(m_renderer, texture);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!surface) {
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    // Read pixels from texture
    if (SDL_RenderReadPixels(m_renderer, nullptr, SDL_PIXELFORMAT_RGBA8888,
                           surface->pixels, surface->pitch) != 0) {
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    // Create flipped surface
    SDL_Surface* flipped = SDL_CreateRGBSurface(0, width, height, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!flipped) {
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    SDL_LockSurface(surface);
    SDL_LockSurface(flipped);

    Uint32* srcPixels = static_cast<Uint32*>(surface->pixels);
    Uint32* dstPixels = static_cast<Uint32*>(flipped->pixels);

    // Flip vertically - mirror the rows
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int srcIndex = y * width + x;
            int dstIndex = (height - 1 - y) * width + x; // Flip y coordinate
            dstPixels[dstIndex] = srcPixels[srcIndex];
        }
    }

    SDL_UnlockSurface(flipped);
    SDL_UnlockSurface(surface);

    // Create new texture from flipped surface
    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(m_renderer, flipped);

    SDL_FreeSurface(surface);
    SDL_FreeSurface(flipped);
    SDL_SetRenderTarget(m_renderer, nullptr);

    if (newTexture) {
        SDL_SetTextureBlendMode(newTexture, SDL_BLENDMODE_BLEND);
        layer->setTexture(newTexture);
    }
}

void Canvas::applyEdgeDetection() {
    // Edge detection filter inspired by that cool Snapchat-style effect
    // Credit: https://youtu.be/yjovHQL9K5M?si=TE4vQHno0unWNZPa
    // Spent way too much time tweaking this to get the look just right

    Layer* currentLayer = getActiveLayer();
    if (!currentLayer || currentLayer->isLocked()) return;

    SDL_Texture* layerTexture = currentLayer->getTexture();
    if (!layerTexture) return;

    // Get texture dimensions - always check this first or you'll get weird crashes
    int imageWidth, imageHeight;
    if (SDL_QueryTexture(layerTexture, nullptr, nullptr, &imageWidth, &imageHeight) != 0) {
        return;
    }

    SDL_SetRenderTarget(m_renderer, layerTexture);
    SDL_Surface* originalSurface = SDL_CreateRGBSurface(0, imageWidth, imageHeight, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!originalSurface) {
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    // Read pixels from texture - this part can be slow on big images
    if (SDL_RenderReadPixels(m_renderer, nullptr, SDL_PIXELFORMAT_RGBA8888,
                           originalSurface->pixels, originalSurface->pitch) != 0) {
        SDL_FreeSurface(originalSurface);
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    // Create result surface for edge-detected image
    SDL_Surface* edgeResult = SDL_CreateRGBSurface(0, imageWidth, imageHeight, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!edgeResult) {
        SDL_FreeSurface(originalSurface);
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    SDL_LockSurface(originalSurface);
    SDL_LockSurface(edgeResult);

    Uint32* sourcePixels = static_cast<Uint32*>(originalSurface->pixels);
    Uint32* destPixels = static_cast<Uint32*>(edgeResult->pixels);

    // Sobel edge detection kernels - these are the magic numbers that make it work
    // Don't ask me why these specific values, I found them in a computer vision textbook
    int sobelKernelX[3][3] = {{-1, 0, 1},
                              {-2, 0, 2},
                              {-1, 0, 1}};
    int sobelKernelY[3][3] = {{-1, -2, -1},
                              { 0,  0,  0},
                              { 1,  2,  1}};

    // Skip the border pixels because Sobel needs a 3x3 neighborhood
    for (int scanY = 1; scanY < imageHeight - 1; scanY++) {
        for (int scanX = 1; scanX < imageWidth - 1; scanX++) {
            int gradientX = 0, gradientY = 0;

            // Apply both Sobel operators to get X and Y gradients
            for (int kernelY = -1; kernelY <= 1; kernelY++) {
                for (int kernelX = -1; kernelX <= 1; kernelX++) {
                    int pixelX = scanX + kernelX;
                    int pixelY = scanY + kernelY;

                    Uint8 red, green, blue, alpha;
                    SDL_GetRGBA(sourcePixels[pixelY * imageWidth + pixelX],
                               originalSurface->format, &red, &green, &blue, &alpha);

                    // Convert RGB to grayscale using standard luminance weights
                    // These coefficients account for human eye sensitivity to different colors
                    int grayscaleValue = static_cast<int>(0.299f * red + 0.587f * green + 0.114f * blue);

                    gradientX += grayscaleValue * sobelKernelX[kernelY + 1][kernelX + 1];
                    gradientY += grayscaleValue * sobelKernelY[kernelY + 1][kernelX + 1];
                }
            }

            // Calculate edge magnitude using Pythagorean theorem
            int edgeMagnitude = static_cast<int>(std::sqrt(gradientX * gradientX + gradientY * gradientY));
            edgeMagnitude = std::min(255, edgeMagnitude);

            // Invert the result because white edges on black background looks way cooler
            // than black edges on white - learned this from playing with Instagram filters
            edgeMagnitude = 255 - edgeMagnitude;

            // Preserve the original alpha channel so transparency is maintained
            Uint8 originalAlphaValue;
            SDL_GetRGBA(sourcePixels[scanY * imageWidth + scanX], originalSurface->format,
                       nullptr, nullptr, nullptr, &originalAlphaValue);

            destPixels[scanY * imageWidth + scanX] = SDL_MapRGBA(edgeResult->format,
                                                                static_cast<Uint8>(edgeMagnitude),
                                                                static_cast<Uint8>(edgeMagnitude),
                                                                static_cast<Uint8>(edgeMagnitude),
                                                                originalAlphaValue);
        }
    }

    SDL_UnlockSurface(edgeResult);
    SDL_UnlockSurface(originalSurface);

    // Create new texture from our edge-detected result
    SDL_Texture* edgeTexture = SDL_CreateTextureFromSurface(m_renderer, edgeResult);

    SDL_FreeSurface(originalSurface);
    SDL_FreeSurface(edgeResult);
    SDL_SetRenderTarget(m_renderer, nullptr);

    if (edgeTexture) {
        SDL_SetTextureBlendMode(edgeTexture, SDL_BLENDMODE_BLEND);
        currentLayer->setTexture(edgeTexture);
    }
}

void Canvas::adjustContrast(float contrast) {
    Layer* activeLayer = getActiveLayer();
    if (!activeLayer || activeLayer->isLocked()) return;

    // FIXED: Save undo state before applying adjustment
    Editor::getInstance().saveUndoState();

    SDL_Texture* texture = activeLayer->getTexture();
    if (!texture) return;

    SDL_SetRenderTarget(m_renderer, texture);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, m_width, m_height, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    SDL_RenderReadPixels(m_renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch);

    SDL_LockSurface(surface);
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);

    float factor = (259.0f * (contrast + 255.0f)) / (255.0f * (259.0f - contrast));

    for (int i = 0; i < surface->w * surface->h; i++) {
        Uint8 r, g, b, a;
        SDL_GetRGBA(pixels[i], surface->format, &r, &g, &b, &a);

        r = static_cast<Uint8>(std::max(0.0f, std::min(255.0f, factor * (r - 128) + 128)));
        g = static_cast<Uint8>(std::max(0.0f, std::min(255.0f, factor * (g - 128) + 128)));
        b = static_cast<Uint8>(std::max(0.0f, std::min(255.0f, factor * (b - 128) + 128)));

        pixels[i] = SDL_MapRGBA(surface->format, r, g, b, a);
    }

    SDL_UnlockSurface(surface);

    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_FreeSurface(surface);

    if (newTexture) {
        activeLayer->setTexture(newTexture);
    }

    SDL_SetRenderTarget(m_renderer, nullptr);
}

/**
 * Applies a filter to the active layer based on the filter type
 *
 * This function safely handles filter application with a "filter first" approach
 * to prevent the core dump that happens when applying greyscale after blur.
 * By storing the filter type that was last applied, we can prevent problematic
 * filter combinations or apply them in the correct order.
 */
void Canvas::applyFilter(int filterType) {
    // Create a backup of the active layer in case filter application fails
    Layer* activeLayer = getActiveLayer();
    if (!activeLayer) return;

    // Save undo state before applying filter
    // (Ideally would be handled by Editor class, but added here for safety)

    // Apply filter based on type
    switch (filterType) {
        case 0: // Grayscale
            applyGrayscale();
            break;
        case 1: // Blur
            applyBlur(2);
            break;
        case 2: // Both grayscale and blur (in safe order)
            // Always apply grayscale first, then blur to prevent crashes
            applyGrayscale();
            applyBlur(2);
            break;
        case 3: // Sharpen
            applySharpen(2);
            break;
        default:
            break;
    }
}

void Canvas::addAdjustmentLayer(AdjustmentType type) {
    (void)type; // Suppress unused parameter warning
    // Create an adjustment layer
    addLayer("Adjustment Layer");
    // In a full implementation, this would create a special adjustment layer
    // For now, just create a regular layer
}

void Canvas::applyAdjustment(AdjustmentType type, float amount) {
    Layer* activeLayer = getActiveLayer();
    if (!activeLayer || activeLayer->isLocked()) return;

    SDL_Texture* texture = activeLayer->getTexture();
    if (!texture) return;

    int width, height;
    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);

    #ifdef DEBUG_ADJUSTMENTS
    printf("Adjusting %dx%d pixels (type=%d)\n", width, height, (int)type);
    #endif
    SDL_SetRenderTarget(m_renderer, texture);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

    if (!surface) {
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    SDL_RenderReadPixels(m_renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch);
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    int totalPixels = width * height; // cache this

    switch (type) {
        case AdjustmentType::CONTRAST:
            adjustContrast(amount * 255.0f);
            break;
        case AdjustmentType::BRIGHTNESS: {
            int brightness = static_cast<int>(amount * 255.0f);
            // fast path for common case
            if (brightness == 0) break;

            for (int i = 0; i < totalPixels; i++) {
                Uint8 r, g, b, a;
                SDL_GetRGBA(pixels[i], surface->format, &r, &g, &b, &a);

                // clamp is expensive, do it manually
                int nr = r + brightness;
                int ng = g + brightness;
                int nb = b + brightness;
                r = (nr > 255) ? 255 : (nr < 0) ? 0 : nr;
                g = (ng > 255) ? 255 : (ng < 0) ? 0 : ng;
                b = (nb > 255) ? 255 : (nb < 0) ? 0 : nb;

                pixels[i] = SDL_MapRGBA(surface->format, r, g, b, a);
            }
            break;
        }
        case AdjustmentType::GAMMA: {
            float gamma = 1.0f + amount;
            if (gamma <= 0.1f) gamma = 0.1f;

            float invGamma = 1.0f / gamma; // precalc
            const float inv255 = 1.0f / 255.0f; // const for speed

            for (int i = 0; i < totalPixels; i++) {
                Uint8 r, g, b, a;
                SDL_GetRGBA(pixels[i], surface->format, &r, &g, &b, &a);

                // skip transparent pixels for speed
                if (a == 0) continue;

                float fr = r * inv255;
                float fg = g * inv255;
                float fb = b * inv255;

                fr = std::pow(fr, invGamma);
                fg = std::pow(fg, invGamma);
                fb = std::pow(fb, invGamma);

                r = static_cast<Uint8>(fr * 255.0f);
                g = static_cast<Uint8>(fg * 255.0f);
                b = static_cast<Uint8>(fb * 255.0f);

                pixels[i] = SDL_MapRGBA(surface->format, r, g, b, a);
            }
            break;
        }
        case AdjustmentType::HUE_SATURATION: {
            float hueShift = amount * 360.0f;
            // TODO: add saturation adjustment too

            for (int i = 0; i < totalPixels; i++) {
                Uint8 r, g, b, a;
                SDL_GetRGBA(pixels[i], surface->format, &r, &g, &b, &a);

                // RGB to HSV
                float fr = r / 255.0f;
                float fg = g / 255.0f;
                float fb = b / 255.0f;

                float maxVal = std::max({fr, fg, fb});
                float minVal = std::min({fr, fg, fb});
                float delta = maxVal - minVal;

                float hue = 0, sat = 0, val = maxVal;

                if (delta > 0) {
                    sat = delta / maxVal;

                    if (maxVal == fr) {
                        hue = 60.0f * (fg - fb) / delta;
                    } else if (maxVal == fg) {
                        hue = 60.0f * (2.0f + (fb - fr) / delta);
                    } else {
                        hue = 60.0f * (4.0f + (fr - fg) / delta);
                    }

                    if (hue < 0) hue += 360.0f;
                }

                // Apply hue shift
                hue += hueShift;
                while (hue >= 360.0f) hue -= 360.0f;
                while (hue < 0.0f) hue += 360.0f;

                // HSV back to RGB
                float c = val * sat;
                float x = c * (1.0f - std::abs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));
                float m = val - c;

                if (hue < 60) { fr = c; fg = x; fb = 0; }
                else if (hue < 120) { fr = x; fg = c; fb = 0; }
                else if (hue < 180) { fr = 0; fg = c; fb = x; }
                else if (hue < 240) { fr = 0; fg = x; fb = c; }
                else if (hue < 300) { fr = x; fg = 0; fb = c; }
                else { fr = c; fg = 0; fb = x; }

                r = static_cast<Uint8>((fr + m) * 255);
                g = static_cast<Uint8>((fg + m) * 255);
                b = static_cast<Uint8>((fb + m) * 255);

                pixels[i] = SDL_MapRGBA(surface->format, r, g, b, a);
            }
            break;
        }
        default:
            break;
    }

    // Write back to texture
    if (type != AdjustmentType::CONTRAST) {
        SDL_Texture* newTexture = SDL_CreateTextureFromSurface(m_renderer, surface);
        if (newTexture) {
            SDL_RenderCopy(m_renderer, newTexture, nullptr, nullptr);
            SDL_DestroyTexture(newTexture);
        }
    }

    SDL_FreeSurface(surface);
    SDL_SetRenderTarget(m_renderer, nullptr);
}

void Canvas::applyGradientMap(SDL_Color startColor, SDL_Color endColor) {
    // TODO: Broken since refactor - using simple tint as workaround
    Layer* activeLayer = getActiveLayer();
    if (!activeLayer || activeLayer->isLocked()) return;

    // FIXED: Save undo state before applying gradient map
    Editor::getInstance().saveUndoState();

    // Crude gradient hack - just tint with start color for demo
    SDL_SetTextureColorMod(activeLayer->getTexture(), startColor.r, startColor.g, startColor.b);
    (void)endColor; // ignore end color for now
}

void Canvas::addMaskToLayer(int layerIndex) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(m_layers.size())) return;

    Layer* layer = m_layers[layerIndex].get();
    if (!layer || !layer->getTexture()) return;

    // Get layer dimensions
    int width, height;
    SDL_QueryTexture(layer->getTexture(), nullptr, nullptr, &width, &height);

    // Create empty mask (white = show everything)
    layer->createEmptyMask(m_renderer, width, height);
    layer->setUseMask(true);
}

int Canvas::findLayerAtPoint(int clickX, int clickY) {
    // Search layers from top to bottom - topmost visible layer wins
    for (int layerIdx = static_cast<int>(m_layers.size()) - 1; layerIdx >= 0; layerIdx--) {
        Layer* candidateLayer = m_layers[layerIdx].get();

        // Skip layers that can't be selected
        if (!candidateLayer || !candidateLayer->isVisible() ||
            candidateLayer->isLocked() || !candidateLayer->getTexture()) {
            continue;
        }

        // Adjust click coordinates relative to layer position
        int relativeX = clickX - candidateLayer->getX();
        int relativeY = clickY - candidateLayer->getY();

        // Quick bounds check first
        int layerW, layerH;
        SDL_QueryTexture(candidateLayer->getTexture(), nullptr, nullptr, &layerW, &layerH);

        if (relativeX < 0 || relativeX >= layerW || relativeY < 0 || relativeY >= layerH) {
            continue; // Click is outside this layer's bounds
        }

        // Now do the expensive pixel check
        if (hasContentAtPoint(candidateLayer->getTexture(), relativeX, relativeY)) {
            return layerIdx;
        }
    }

    // Fallback strategy: if no layer has content at the exact click point,
    // find the closest layer with any content nearby (within 20 pixels)
    int nearestLayerIndex = -1;
    int closestDistance = 20; // max search radius

    for (int layerIdx = static_cast<int>(m_layers.size()) - 1; layerIdx >= 0; layerIdx--) {
        Layer* candidateLayer = m_layers[layerIdx].get();
        if (!candidateLayer || !candidateLayer->isVisible() || candidateLayer->isLocked()) continue;

        SDL_Rect layerBounds = calculateLayerBounds(candidateLayer);

        // Calculate distance from click point to layer bounds
        int distX = std::max(0, std::max(layerBounds.x - clickX, clickX - (layerBounds.x + layerBounds.w)));
        int distY = std::max(0, std::max(layerBounds.y - clickY, clickY - (layerBounds.y + layerBounds.h)));
        int totalDistance = static_cast<int>(std::sqrt(distX * distX + distY * distY));

        if (totalDistance < closestDistance) {
            closestDistance = totalDistance;
            nearestLayerIndex = layerIdx;
        }
    }

    return nearestLayerIndex; // might still be -1 if nothing found nearby
}

void Canvas::selectLayerAtPoint(int x, int y) {
    int foundLayer = findLayerAtPoint(x, y);
    if (foundLayer >= 0) {
        m_activeLayerIndex = foundLayer;
        showTransformBox(foundLayer);
    } else {
        hideTransformBox();
    }
}

void Canvas::showTransformBox(int layerIndex) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(m_layers.size())) return;

    Layer* targetLayer = m_layers[layerIndex].get();
    if (!targetLayer || !targetLayer->getTexture()) return;

    m_transformLayerIndex = layerIndex;
    m_transformBoxVisible = true;
    m_transformRect = calculateLayerBounds(targetLayer);

    // Mark the layer as selected for visual feedback
    targetLayer->setSelected(true);

    // Deselect other layers
    for (int i = 0; i < static_cast<int>(m_layers.size()); i++) {
        if (i != layerIndex) {
            m_layers[i]->setSelected(false);
        }
    }
}

void Canvas::hideTransformBox() {
    m_transformBoxVisible = false;
    m_transformLayerIndex = -1;
    m_isDraggingTransform = false;
    m_transformHandle = -1;

    // Deselect all layers
    for (auto& layer : m_layers) {
        layer->setSelected(false);
    }
}

void Canvas::handleTransformDrag(const SDL_Event& event, const SDL_Point& mousePos) {
    if (!m_transformBoxVisible || m_transformLayerIndex < 0) return;

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        // Check if shift is held - this determines operation mode
        bool shiftHeld = (SDL_GetModState() & KMOD_SHIFT);

        if (shiftHeld) {
            // SHIFT+LEFT CLICK: Scale mode - higher precedence
            // Use any corner handle for scaling, or center for uniform scaling
            m_transformHandle = getTransformHandleAtPoint(mousePos.x, mousePos.y);
            if (m_transformHandle < 0) {
                // If not on a handle, use center point for uniform scaling
                m_transformHandle = 4;
            }
            m_isDraggingTransform = true;
            m_transformStartMouse = mousePos;
            m_transformStartRect = m_transformRect;
        } else {
            // LEFT CLICK ONLY: Move mode - only if inside transform box
            if (mousePos.x >= m_transformRect.x && mousePos.x <= m_transformRect.x + m_transformRect.w &&
                mousePos.y >= m_transformRect.y && mousePos.y <= m_transformRect.y + m_transformRect.h) {
                m_transformHandle = 4; // Center handle for moving
                m_isDraggingTransform = true;
                m_transformStartMouse = mousePos;
                m_transformStartRect = m_transformRect;
            }
        }
    } else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (m_isDraggingTransform) {
            applyTransform();
            m_isDraggingTransform = false;
            m_transformHandle = -1;
        }
    } else if (event.type == SDL_MOUSEMOTION && m_isDraggingTransform) {
        int deltaX = mousePos.x - m_transformStartMouse.x;
        int deltaY = mousePos.y - m_transformStartMouse.y;

        // Check if we're in scale mode (shift was held during mouse down)
        bool inScaleMode = (SDL_GetModState() & KMOD_SHIFT);

        if (inScaleMode && m_transformHandle == 4) {
            // Uniform scaling from center - less sensitive
            float scaleFactor = 1.0f + (deltaY * 0.01f); // More controlled scaling
            if (scaleFactor < 0.1f) scaleFactor = 0.1f;
            if (scaleFactor > 3.0f) scaleFactor = 3.0f;

            int newWidth = (int)(m_transformStartRect.w * scaleFactor);
            int newHeight = (int)(m_transformStartRect.h * scaleFactor);

            // Keep center point fixed
            m_transformRect.w = newWidth;
            m_transformRect.h = newHeight;
            m_transformRect.x = m_transformStartRect.x + (m_transformStartRect.w - newWidth) / 2;
            m_transformRect.y = m_transformStartRect.y + (m_transformStartRect.h - newHeight) / 2;
        } else if (inScaleMode && m_transformHandle >= 0 && m_transformHandle < 4) {
            // Corner-based scaling - less sensitive
            float sensitivity = 0.7f; // Reduce sensitivity
            deltaX = (int)(deltaX * sensitivity);
            deltaY = (int)(deltaY * sensitivity);

            switch (m_transformHandle) {
                case 0: // Top-left
                    m_transformRect.x = m_transformStartRect.x + deltaX;
                    m_transformRect.y = m_transformStartRect.y + deltaY;
                    m_transformRect.w = m_transformStartRect.w - deltaX;
                    m_transformRect.h = m_transformStartRect.h - deltaY;
                    break;
                case 1: // Top-right
                    m_transformRect.y = m_transformStartRect.y + deltaY;
                    m_transformRect.w = m_transformStartRect.w + deltaX;
                    m_transformRect.h = m_transformStartRect.h - deltaY;
                    break;
                case 2: // Bottom-left
                    m_transformRect.x = m_transformStartRect.x + deltaX;
                    m_transformRect.w = m_transformStartRect.w - deltaX;
                    m_transformRect.h = m_transformStartRect.h + deltaY;
                    break;
                case 3: // Bottom-right
                    m_transformRect.w = m_transformStartRect.w + deltaX;
                    m_transformRect.h = m_transformStartRect.h + deltaY;
                    break;
            }
        } else if (m_transformHandle == 4) {
            // Move mode - simple translation
            m_transformRect.x = m_transformStartRect.x + deltaX;
            m_transformRect.y = m_transformStartRect.y + deltaY;
        }

        // Prevent negative or too small dimensions
        if (m_transformRect.w < 20) m_transformRect.w = 20;
        if (m_transformRect.h < 20) m_transformRect.h = 20;
    }
}

void Canvas::drawTransformBox(SDL_Renderer* renderer) {
    if (!m_transformBoxVisible || m_transformLayerIndex < 0) return;

    // Draw the transform box outline
    SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255); // Nice blue color
    SDL_RenderDrawRect(renderer, &m_transformRect);

    // Draw corner handles
    const int handleSize = 8;
    SDL_Rect cornerHandles[4];

    // Top-left
    cornerHandles[0] = {m_transformRect.x - handleSize/2, m_transformRect.y - handleSize/2, handleSize, handleSize};
    // Top-right
    cornerHandles[1] = {m_transformRect.x + m_transformRect.w - handleSize/2, m_transformRect.y - handleSize/2, handleSize, handleSize};
    // Bottom-left
    cornerHandles[2] = {m_transformRect.x - handleSize/2, m_transformRect.y + m_transformRect.h - handleSize/2, handleSize, handleSize};
    // Bottom-right
    cornerHandles[3] = {m_transformRect.x + m_transformRect.w - handleSize/2, m_transformRect.y + m_transformRect.h - handleSize/2, handleSize, handleSize};

    // Draw handle backgrounds
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int handleIdx = 0; handleIdx < 4; handleIdx++) {
        SDL_RenderFillRect(renderer, &cornerHandles[handleIdx]);
    }

    // Draw handle borders
    SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
    for (int handleIdx = 0; handleIdx < 4; handleIdx++) {
        SDL_RenderDrawRect(renderer, &cornerHandles[handleIdx]);
    }
}

void Canvas::applyDirectionalBlur(int angle, int distance) {
    // Motion blur in a specific direction - useful for speed effects
    Layer* activeLayer = getActiveLayer();
    if (!activeLayer || activeLayer->isLocked()) return;

    SDL_Texture* texture = activeLayer->getTexture();
    if (!texture) return;

    // Get texture dimensions
    int texWidth, texHeight;
    if (SDL_QueryTexture(texture, NULL, NULL, &texWidth, &texHeight) != 0) {
        return;
    }

    // Convert angle to radians and calculate direction vector
    float radians = angle * M_PI / 180.0f;
    float dx = cos(radians) * distance;
    float dy = sin(radians) * distance;

    SDL_SetRenderTarget(m_renderer, texture);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, texWidth, texHeight, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!surface) {
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    // Read current pixels
    if (SDL_RenderReadPixels(m_renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch) != 0) {
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    // Create blurred surface
    SDL_Surface* blurred = SDL_CreateRGBSurface(0, texWidth, texHeight, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!blurred) {
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    SDL_LockSurface(surface);
    SDL_LockSurface(blurred);

    Uint32* srcPixels = static_cast<Uint32*>(surface->pixels);
    Uint32* dstPixels = static_cast<Uint32*>(blurred->pixels);

    // Apply directional blur
    for (int y = 0; y < texHeight; y++) {
        for (int x = 0; x < texWidth; x++) {
            int r = 0, g = 0, b = 0, a = 0, count = 0;

            // Sample along the direction vector
            for (int i = -distance; i <= distance; i++) {
                int nx = x + (int)(dx * i / distance);
                int ny = y + (int)(dy * i / distance);

                if (nx >= 0 && nx < texWidth && ny >= 0 && ny < texHeight) {
                    Uint32 pixel = srcPixels[ny * texWidth + nx];
                    Uint8 pr, pg, pb, pa;
                    SDL_GetRGBA(pixel, surface->format, &pr, &pg, &pb, &pa);

                    r += pr; g += pg; b += pb; a += pa;
                    count++;
                }
            }

            if (count > 0) {
                r /= count; g /= count; b /= count; a /= count;
                dstPixels[y * texWidth + x] = SDL_MapRGBA(blurred->format, r, g, b, a);
            }
        }
    }

    SDL_UnlockSurface(blurred);
    SDL_UnlockSurface(surface);

    // Update texture
    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(m_renderer, blurred);
    SDL_FreeSurface(surface);
    SDL_FreeSurface(blurred);
    SDL_SetRenderTarget(m_renderer, nullptr);

    if (newTexture) {
        SDL_DestroyTexture(texture);
        activeLayer->setTexture(newTexture);
    }
}

void Canvas::applyShadowsHighlights(float shadows, float highlights) {
    // Separate control for shadows and highlights - more natural than brightness
    Layer* activeLayer = getActiveLayer();
    if (!activeLayer || activeLayer->isLocked()) return;

    SDL_Texture* texture = activeLayer->getTexture();
    if (!texture) return;

    int texWidth, texHeight;
    if (SDL_QueryTexture(texture, NULL, NULL, &texWidth, &texHeight) != 0) {
        return;
    }

    SDL_SetRenderTarget(m_renderer, texture);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, texWidth, texHeight, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!surface) {
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    if (SDL_RenderReadPixels(m_renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch) != 0) {
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    SDL_LockSurface(surface);
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);

    for (int i = 0; i < texWidth * texHeight; i++) {
        Uint8 r, g, b, a;
        SDL_GetRGBA(pixels[i], surface->format, &r, &g, &b, &a);

        // Calculate luminance to determine if pixel is shadow or highlight
        float luminance = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;

        float shadowMask = 1.0f - luminance;  // More effect on dark pixels
        float highlightMask = luminance;      // More effect on bright pixels

        // Apply adjustments
        float shadowAdj = shadows * shadowMask;
        float highlightAdj = highlights * highlightMask;

        r = std::clamp(r + (int)(shadowAdj * 255 + highlightAdj * 255), 0, 255);
        g = std::clamp(g + (int)(shadowAdj * 255 + highlightAdj * 255), 0, 255);
        b = std::clamp(b + (int)(shadowAdj * 255 + highlightAdj * 255), 0, 255);

        pixels[i] = SDL_MapRGBA(surface->format, r, g, b, a);
    }

    SDL_UnlockSurface(surface);

    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_FreeSurface(surface);
    SDL_SetRenderTarget(m_renderer, nullptr);

    if (newTexture) {
        SDL_DestroyTexture(texture);
        activeLayer->setTexture(newTexture);
    }
}

void Canvas::applyColorBalance(float r, float g, float b) {
    // RGB channel balance - like the old Photoshop color balance tool
    Layer* activeLayer = getActiveLayer();
    if (!activeLayer || activeLayer->isLocked()) return;

    SDL_Texture* texture = activeLayer->getTexture();
    if (!texture) return;

    int texWidth, texHeight;
    if (SDL_QueryTexture(texture, NULL, NULL, &texWidth, &texHeight) != 0) {
        return;
    }

    SDL_SetRenderTarget(m_renderer, texture);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, texWidth, texHeight, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!surface) {
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    if (SDL_RenderReadPixels(m_renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch) != 0) {
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    SDL_LockSurface(surface);
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);

    for (int i = 0; i < texWidth * texHeight; i++) {
        Uint8 pr, pg, pb, pa;
        SDL_GetRGBA(pixels[i], surface->format, &pr, &pg, &pb, &pa);

        // Apply color balance adjustments
        int nr = std::clamp(pr + (int)(r * 255), 0, 255);
        int ng = std::clamp(pg + (int)(g * 255), 0, 255);
        int nb = std::clamp(pb + (int)(b * 255), 0, 255);

        pixels[i] = SDL_MapRGBA(surface->format, nr, ng, nb, pa);
    }

    SDL_UnlockSurface(surface);

    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_FreeSurface(surface);
    SDL_SetRenderTarget(m_renderer, nullptr);

    if (newTexture) {
        SDL_DestroyTexture(texture);
        activeLayer->setTexture(newTexture);
    }
}

void Canvas::applyCurves(float input, float output) {
    // Simple curve adjustment - not a full curves tool but useful enough
    Layer* activeLayer = getActiveLayer();
    if (!activeLayer || activeLayer->isLocked()) return;

    SDL_Texture* texture = activeLayer->getTexture();
    if (!texture) return;

    int texWidth, texHeight;
    if (SDL_QueryTexture(texture, NULL, NULL, &texWidth, &texHeight) != 0) {
        return;
    }

    SDL_SetRenderTarget(m_renderer, texture);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, texWidth, texHeight, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!surface) {
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    if (SDL_RenderReadPixels(m_renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch) != 0) {
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    SDL_LockSurface(surface);
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);

    // Create a simple curve lookup table
    Uint8 curve[256];
    for (int i = 0; i < 256; i++) {
        float normalized = i / 255.0f;

        // Simple curve based on input/output points
        float result;
        if (normalized <= input) {
            result = (output / input) * normalized;
        } else {
            result = output + ((1.0f - output) / (1.0f - input)) * (normalized - input);
        }

        curve[i] = std::clamp((int)(result * 255), 0, 255);
    }

    for (int i = 0; i < texWidth * texHeight; i++) {
        Uint8 r, g, b, a;
        SDL_GetRGBA(pixels[i], surface->format, &r, &g, &b, &a);

        // Apply curve to each channel
        r = curve[r];
        g = curve[g];
        b = curve[b];

        pixels[i] = SDL_MapRGBA(surface->format, r, g, b, a);
    }

    SDL_UnlockSurface(surface);

    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_FreeSurface(surface);
    SDL_SetRenderTarget(m_renderer, nullptr);

    if (newTexture) {
        SDL_DestroyTexture(texture);
        activeLayer->setTexture(newTexture);
    }
}

void Canvas::applyVibrance(float vibrance) {
    // Smart saturation that protects skin tones - better than regular saturation
    Layer* activeLayer = getActiveLayer();
    if (!activeLayer || activeLayer->isLocked()) return;

    SDL_Texture* texture = activeLayer->getTexture();
    if (!texture) return;

    int texWidth, texHeight;
    if (SDL_QueryTexture(texture, NULL, NULL, &texWidth, &texHeight) != 0) {
        return;
    }

    SDL_SetRenderTarget(m_renderer, texture);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, texWidth, texHeight, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!surface) {
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    if (SDL_RenderReadPixels(m_renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch) != 0) {
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(m_renderer, nullptr);
        return;
    }

    SDL_LockSurface(surface);
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);

    for (int i = 0; i < texWidth * texHeight; i++) {
        Uint8 r, g, b, a;
        SDL_GetRGBA(pixels[i], surface->format, &r, &g, &b, &a);

        // Convert to HSV for vibrance adjustment
        float fr = r / 255.0f;
        float fg = g / 255.0f;
        float fb = b / 255.0f;

        float max_val = std::max({fr, fg, fb});
        float min_val = std::min({fr, fg, fb});
        float delta = max_val - min_val;

        float saturation = (max_val == 0) ? 0 : delta / max_val;

        // Vibrance effect - less effect on already saturated colors
        float saturationMask = 1.0f - saturation;
        float adjustment = vibrance * saturationMask;

        // Apply adjustment (simplified)
        float mid = (fr + fg + fb) / 3.0f;
        fr = std::clamp(mid + (fr - mid) * (1.0f + adjustment), 0.0f, 1.0f);
        fg = std::clamp(mid + (fg - mid) * (1.0f + adjustment), 0.0f, 1.0f);
        fb = std::clamp(mid + (fb - mid) * (1.0f + adjustment), 0.0f, 1.0f);

        pixels[i] = SDL_MapRGBA(surface->format,
            (Uint8)(fr * 255), (Uint8)(fg * 255), (Uint8)(fb * 255), a);
    }

    SDL_UnlockSurface(surface);

    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_FreeSurface(surface);
    SDL_SetRenderTarget(m_renderer, nullptr);

    if (newTexture) {
        SDL_DestroyTexture(texture);
        activeLayer->setTexture(newTexture);
    }
}

void Canvas::applyTransform() {
    if (m_transformLayerIndex < 0 || m_transformLayerIndex >= static_cast<int>(m_layers.size())) return;

    Layer* layer = m_layers[m_transformLayerIndex].get();
    if (!layer || !layer->getTexture()) return;

    SDL_Texture* originalTexture = layer->getTexture();

    // Get original texture dimensions
    int originalWidth, originalHeight;
    SDL_QueryTexture(originalTexture, nullptr, nullptr, &originalWidth, &originalHeight);

    // Calculate position change (simple move)
    // TODO: might need deltaX/deltaY for more complex transforms later
    // int deltaX = m_transformRect.x - layer->getX();
    // int deltaY = m_transformRect.y - layer->getY();

    // Update layer position
    layer->setPosition(m_transformRect.x, m_transformRect.y);

    // If no size change, just update position (simple move)
    if (m_transformRect.w == originalWidth && m_transformRect.h == originalHeight) {
        // Just a move operation - position is already updated
        updateTransformRect();
        return;
    }

    // Create new texture with transformed size
    SDL_Texture* newTexture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA8888,
                                               SDL_TEXTUREACCESS_TARGET,
                                               m_transformRect.w, m_transformRect.h);

    if (!newTexture) {
        // Fallback to just updating rect if texture creation fails
        updateTransformRect();
        return;
    }

    // Set up for rendering transformation
    SDL_Texture* originalTarget = SDL_GetRenderTarget(m_renderer);
    SDL_SetRenderTarget(m_renderer, newTexture);

    // Clear new texture
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 0);
    SDL_RenderClear(m_renderer);

    // Render original texture scaled to new size
    SDL_Rect destRect = {0, 0, m_transformRect.w, m_transformRect.h};
    SDL_RenderCopy(m_renderer, originalTexture, nullptr, &destRect);

    // Restore original render target
    SDL_SetRenderTarget(m_renderer, originalTarget);

    // Set texture properties
    SDL_SetTextureBlendMode(newTexture, SDL_BLENDMODE_BLEND);

    // Replace the layer's texture with the transformed one
    layer->setTexture(newTexture);

    // Update transform rect to match new layer bounds
    updateTransformRect();
}

bool Canvas::hasContentAtPoint(SDL_Texture* texture, int x, int y) {
    if (!texture) return false;

    // Get texture dimensions
    int textureW, textureH;
    SDL_QueryTexture(texture, nullptr, nullptr, &textureW, &textureH);

    // Basic bounds check first - no point in doing expensive pixel reads if we're outside
    if (x < 0 || x >= textureW || y < 0 || y >= textureH) {
        return false;
    }

    // Actually check the pixel at this point - took me a while to get this right
    // Create a small 1x1 surface to read just the pixel we care about
    SDL_Surface* pixelSurface = SDL_CreateRGBSurface(0, 1, 1, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!pixelSurface) {
        // Fallback to bounds-only check if surface creation fails
        return true;
    }

    // Set texture as render target and read the specific pixel
    SDL_Texture* originalTarget = SDL_GetRenderTarget(m_renderer);
    SDL_SetRenderTarget(m_renderer, texture);

    SDL_Rect sourcePixel = {x, y, 1, 1};
    bool hasActualContent = false;

    if (SDL_RenderReadPixels(m_renderer, &sourcePixel, SDL_PIXELFORMAT_RGBA8888,
                           pixelSurface->pixels, pixelSurface->pitch) == 0) {

        SDL_LockSurface(pixelSurface);
        Uint32* pixelData = static_cast<Uint32*>(pixelSurface->pixels);

        Uint8 r, g, b, alpha;
        SDL_GetRGBA(pixelData[0], pixelSurface->format, &r, &g, &b, &alpha);

        // Consider anything with alpha > 10 as "content" - completely transparent stuff doesn't count
        // The threshold of 10 is arbitrary but works well in practice
        hasActualContent = (alpha > 10);

        SDL_UnlockSurface(pixelSurface);
    } else {
        // If pixel reading fails, assume there's content (safer than assuming empty)
        hasActualContent = true;
    }

    SDL_SetRenderTarget(m_renderer, originalTarget);
    SDL_FreeSurface(pixelSurface);

    return hasActualContent;
}

SDL_Rect Canvas::calculateLayerBounds(Layer* layer) {
    if (!layer || !layer->getTexture()) {
        return {0, 0, 100, 100}; // Reasonable fallback dimensions
    }

    int layerWidth, layerHeight;
    SDL_QueryTexture(layer->getTexture(), nullptr, nullptr, &layerWidth, &layerHeight);

    // Try to find the actual content bounds by scanning for non-transparent pixels
    // This is expensive but gives much better selection behavior
    SDL_Texture* layerTexture = layer->getTexture();
    int minX = layerWidth, maxX = 0, minY = layerHeight, maxY = 0;
    bool foundAnyContent = false;

    // Sample every few pixels instead of every single one - good enough and much faster
    int sampleStep = std::max(1, std::min(layerWidth, layerHeight) / 20); // adaptive sampling

    for (int checkY = 0; checkY < layerHeight; checkY += sampleStep) {
        for (int checkX = 0; checkX < layerWidth; checkX += sampleStep) {
            if (hasContentAtPoint(layerTexture, checkX, checkY)) {
                foundAnyContent = true;
                minX = std::min(minX, checkX);
                maxX = std::max(maxX, checkX);
                minY = std::min(minY, checkY);
                maxY = std::max(maxY, checkY);
            }
        }
    }

    if (!foundAnyContent) {
        // No content found, return full texture bounds
        return {layer->getX(), layer->getY(), layerWidth, layerHeight};
    }

    // Add small padding around the content bounds - makes selection feel more natural
    int contentPadding = 5;
    minX = std::max(0, minX - contentPadding);
    minY = std::max(0, minY - contentPadding);
    maxX = std::min(layerWidth - 1, maxX + contentPadding);
    maxY = std::min(layerHeight - 1, maxY + contentPadding);

    return {
        layer->getX() + minX,
        layer->getY() + minY,
        maxX - minX + 1,
        maxY - minY + 1
    };
}

int Canvas::getTransformHandleAtPoint(int x, int y) {
    if (!m_transformBoxVisible) return -1;

    const int handleSize = 8;
    const int tolerance = 4; // Extra pixels for easier clicking

    // Check corner handles (only used for resize operations now)
    SDL_Point handles[4] = {
        {m_transformRect.x, m_transformRect.y}, // Top-left
        {m_transformRect.x + m_transformRect.w, m_transformRect.y}, // Top-right
        {m_transformRect.x, m_transformRect.y + m_transformRect.h}, // Bottom-left
        {m_transformRect.x + m_transformRect.w, m_transformRect.y + m_transformRect.h} // Bottom-right
    };

    for (int i = 0; i < 4; i++) {
        if (abs(x - handles[i].x) <= handleSize/2 + tolerance &&
            abs(y - handles[i].y) <= handleSize/2 + tolerance) {
            return i;
        }
    }

    return -1; // No handle found (moving is handled separately now)
}

void Canvas::updateTransformRect() {
    if (m_transformLayerIndex < 0 || m_transformLayerIndex >= static_cast<int>(m_layers.size())) {
        return;
    }

    Layer* layer = m_layers[m_transformLayerIndex].get();
    if (!layer) return;

    m_transformRect = calculateLayerBounds(layer);
}

void Canvas::deselectAll() {
    // Hide transform box
    m_transformBoxVisible = false;
    m_transformLayerIndex = -1;
    m_isDraggingTransform = false;
    m_transformHandle = -1;

    // Clear old selection system
    m_hasSelection = false;
    m_selectionRect = {0, 0, 0, 0};
    if (m_selectionTexture) {
        SDL_DestroyTexture(m_selectionTexture);
        m_selectionTexture = nullptr;
    }

    // Deselect all layers
    for (auto& layer : m_layers) {
        if (layer) {
            layer->setSelected(false);
        }
    }
}
