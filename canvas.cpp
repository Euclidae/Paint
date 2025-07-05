#include "canvas.hpp"
#include "tools.hpp"
#include <algorithm>
#include <iostream>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

int CANVAS_WIDTH = 1280;
int CANVAS_HEIGHT = 720;
std::vector<Canvas::Layer> Canvas::layers;
int Canvas::activeLayerIndex = 0;
SDL_Texture* Canvas::canvasBuffer = nullptr;
SDL_Renderer* Canvas::renderer = nullptr;
std::map<int, TTF_Font*> Canvas::fontCache;

SDL_Rect selectionRect = {0, 0, 0, 0};
bool hasSelection = false;
SDL_Texture* selectionTexture = nullptr;
ImVec2 selectionOffset = ImVec2(0, 0);

void Canvas::init(SDL_Renderer* rend) {
    renderer = rend;
    setupNewCanvas(CANVAS_WIDTH, CANVAS_HEIGHT);
    addLayer("Background");
}

void Canvas::cleanup() {
    for (auto& layer : layers) {
        if (layer.texture) SDL_DestroyTexture(layer.texture);
    }
    if (canvasBuffer) SDL_DestroyTexture(canvasBuffer);
    clearFontCache();
}

void Canvas::clearFontCache() {
    for (auto& fontPair : fontCache) {
        if (fontPair.second) TTF_CloseFont(fontPair.second);
    }
    fontCache.clear();
}

TTF_Font* Canvas::getFont(int size, bool bold, bool italic) {
    int key = (size << 2) | (bold ? 1 : 0) | (italic ? 2 : 0);
    if (fontCache.find(key) != fontCache.end()) return fontCache[key];
    
    // Try multiple font paths - Common system fonts
    const char* fontPaths[] = {
        "arial.ttf", "DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",  // Common Linux path
        "C:/Windows/Fonts/arial.ttf"                       // Common Windows path
    };
    
    TTF_Font* font = nullptr;
    for (const char* path : fontPaths) {
        font = TTF_OpenFont(path, size);
        if (font) break;
    }
    
    if (!font) {
        std::cerr << "Failed to load font! Error: " << TTF_GetError() << std::endl;
        return nullptr;
    }
    
    int style = TTF_STYLE_NORMAL;
    if (bold) style |= TTF_STYLE_BOLD;
    if (italic) style |= TTF_STYLE_ITALIC;
    TTF_SetFontStyle(font, style);
    
    fontCache[key] = font;
    return font;
}

void Canvas::createNewLayerTexture(Layer& layer) {
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

void Canvas::setupNewCanvas(int width, int height) {
    CANVAS_WIDTH = width;
    CANVAS_HEIGHT = height;
    if (canvasBuffer) SDL_DestroyTexture(canvasBuffer);
    canvasBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET, CANVAS_WIDTH, CANVAS_HEIGHT);
    SDL_SetTextureBlendMode(canvasBuffer, SDL_BLENDMODE_BLEND);
    for (auto& layer : layers) {
        createNewLayerTexture(layer);
    }
}

void Canvas::addLayer(const std::string& name, bool) {
    Layer newLayer;
    newLayer.name = name;
    createNewLayerTexture(newLayer);
    layers.push_back(newLayer);
    activeLayerIndex = static_cast<int>(layers.size()) - 1;
}

// Text layer functionality removed - using new text tool instead

void Canvas::moveLayer(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= static_cast<int>(layers.size()) ||
        toIndex < 0 || toIndex >= static_cast<int>(layers.size()) || fromIndex == toIndex) return;
    Layer movedLayer = layers[fromIndex];
    layers.erase(layers.begin() + fromIndex);
    if (toIndex > fromIndex) toIndex--;
    layers.insert(layers.begin() + toIndex, movedLayer);
    if (activeLayerIndex == fromIndex) activeLayerIndex = toIndex;
    else if (fromIndex < toIndex && activeLayerIndex > fromIndex && activeLayerIndex <= toIndex) activeLayerIndex--;
    else if (fromIndex > toIndex && activeLayerIndex >= toIndex && activeLayerIndex < fromIndex) activeLayerIndex++;
}

void Canvas::duplicateLayer(int index) {
    if (index < 0 || index >= static_cast<int>(layers.size())) return;
    Layer dupLayer = layers[index];
    dupLayer.name += " Copy";
    createNewLayerTexture(dupLayer);
    SDL_SetRenderTarget(renderer, dupLayer.texture);
    SDL_RenderCopy(renderer, layers[index].texture, nullptr, nullptr);
    SDL_SetRenderTarget(renderer, nullptr);
    // Text layer functionality removed - using new text tool
    layers.insert(layers.begin() + index + 1, dupLayer);
    activeLayerIndex = index + 1;
}

void Canvas::removeLayer(int index) {
    if (index < 0 || index >= static_cast<int>(layers.size())) return;
    if (layers[index].texture) SDL_DestroyTexture(layers[index].texture);
    // Text layer cleanup removed - using new text tool
    layers.erase(layers.begin() + index);
    if (activeLayerIndex >= static_cast<int>(layers.size())) activeLayerIndex = static_cast<int>(layers.size()) - 1;
}

void Canvas::importImage(const char* filePath) {
    SDL_Surface* loadedSurface = IMG_Load(filePath);
    if (!loadedSurface) {
        std::cerr << "IMG_Load failed: " << IMG_GetError() << std::endl;
        return;
    }
    addLayer("Imported");
    Layer& layer = layers[activeLayerIndex];
    SDL_Texture* imgTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
    if (!imgTexture) {
        std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(loadedSurface);
        return;
    }
    SDL_SetRenderTarget(renderer, layer.texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_Rect destRect = {0, 0, CANVAS_WIDTH, CANVAS_HEIGHT};
    SDL_RenderCopy(renderer, imgTexture, nullptr, &destRect);
    SDL_SetRenderTarget(renderer, nullptr);
    SDL_DestroyTexture(imgTexture);
    SDL_FreeSurface(loadedSurface);
}

void Canvas::exportImage(const char* filePath, const char* format) {
    SDL_Surface* surface = SDL_CreateRGBSurface(0, CANVAS_WIDTH, CANVAS_HEIGHT, 32,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
    if (!surface) {
        std::cerr << "Failed to create surface: " << SDL_GetError() << std::endl;
        return;
    }
    SDL_SetRenderTarget(renderer, canvasBuffer);
    SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch);
    SDL_SetRenderTarget(renderer, nullptr);
    std::string formatStr(format);
    std::transform(formatStr.begin(), formatStr.end(), formatStr.begin(), ::tolower);
    if (formatStr == "png") {
        IMG_SavePNG(surface, filePath);
    } else if (formatStr == "jpg" || formatStr == "jpeg") {
        IMG_SaveJPG(surface, filePath, 90);
    } else if (formatStr == "bmp") {
        SDL_SaveBMP(surface, filePath);
    } else {
        std::cerr << "Unsupported format: " << format << std::endl;
    }
    SDL_FreeSurface(surface);
}

void Canvas::render() {
    SDL_SetRenderTarget(renderer, canvasBuffer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
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
        // Text layers removed - using new text tool
    }
    SDL_SetRenderTarget(renderer, nullptr);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderClear(renderer);
    SDL_Rect canvasRect = {10, 10, CANVAS_WIDTH, CANVAS_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &canvasRect);
    canvasRect.x += 1; canvasRect.y += 1; canvasRect.w -= 2; canvasRect.h -= 2;
    SDL_RenderCopy(renderer, canvasBuffer, nullptr, &canvasRect);

    // --- Draw text tool preview while dragging ---
    if (Tools::isDrawing && Tools::currentTool == TEXT) {
        SDL_Rect textPreview = {
            (int)std::min(Tools::startPos.x, Tools::currentPos.x) + 10,
            (int)std::min(Tools::startPos.y, Tools::currentPos.y) + 10,
            (int)std::abs(Tools::currentPos.x - Tools::startPos.x),
            (int)std::abs(Tools::currentPos.y - Tools::startPos.y)
        };
        // Draw semi-transparent background
        SDL_SetRenderDrawColor(renderer, 200, 200, 255, 64);
        SDL_RenderFillRect(renderer, &textPreview);
        // Draw dashed border
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderDrawRect(renderer, &textPreview);
        
        // Draw "TEXT BOX" instruction in center
        if (textPreview.w > 60 && textPreview.h > 20) {
            // Simple text instruction without font rendering
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            int centerX = textPreview.x + textPreview.w / 2;
            int centerY = textPreview.y + textPreview.h / 2;
            // Draw a simple "T" shape to indicate text
            SDL_RenderDrawLine(renderer, centerX - 8, centerY - 8, centerX + 8, centerY - 8);
            SDL_RenderDrawLine(renderer, centerX, centerY - 8, centerX, centerY + 8);
        }
    }

    // --- Draw select tool preview while dragging ---
    if (Tools::isDrawing && Tools::currentTool == SELECT) {
        SDL_Rect selectionPreview = {
            (int)std::min(Tools::startPos.x, Tools::currentPos.x) + 10,
            (int)std::min(Tools::startPos.y, Tools::currentPos.y) + 10,
            (int)std::abs(Tools::currentPos.x - Tools::startPos.x),
            (int)std::abs(Tools::currentPos.y - Tools::startPos.y)
        };
        // Draw selection rectangle with dashed border effect
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
        SDL_RenderFillRect(renderer, &selectionPreview);
        // Draw marching ants effect (simplified)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &selectionPreview);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        // Draw inner dashed lines
        for (int i = 0; i < selectionPreview.w; i += 6) {
            SDL_RenderDrawPoint(renderer, selectionPreview.x + i, selectionPreview.y);
            SDL_RenderDrawPoint(renderer, selectionPreview.x + i, selectionPreview.y + selectionPreview.h - 1);
        }
        for (int i = 0; i < selectionPreview.h; i += 6) {
            SDL_RenderDrawPoint(renderer, selectionPreview.x, selectionPreview.y + i);
            SDL_RenderDrawPoint(renderer, selectionPreview.x + selectionPreview.w - 1, selectionPreview.y + i);
        }
    }

    // --- Render text boxes ---
    Tools::renderTextBoxes(renderer);

    // --- Draw selection if present ---
    if (hasSelection && selectionTexture) {
        SDL_Rect dest = selectionRect;
        dest.x += (int)selectionOffset.x + 10;
        dest.y += (int)selectionOffset.y + 10;
        SDL_RenderCopy(renderer, selectionTexture, nullptr, &dest);

        // Draw selection rectangle with marching ants effect
        SDL_SetRenderDrawColor(renderer, 0, 120, 255, 255);
        SDL_RenderDrawRect(renderer, &dest);
        
        // Draw corner handles for selection
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        int handleSize = 6;
        SDL_Rect handles[4] = {
            {dest.x - handleSize/2, dest.y - handleSize/2, handleSize, handleSize},
            {dest.x + dest.w - handleSize/2, dest.y - handleSize/2, handleSize, handleSize},
            {dest.x - handleSize/2, dest.y + dest.h - handleSize/2, handleSize, handleSize},
            {dest.x + dest.w - handleSize/2, dest.y + dest.h - handleSize/2, handleSize, handleSize}
        };
        for (int i = 0; i < 4; i++) {
            SDL_RenderFillRect(renderer, &handles[i]);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &handles[i]);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        }
    }
}