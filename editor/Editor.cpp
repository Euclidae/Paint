#include "Editor.hpp"
#include "../canvas/Canvas.hpp"
#include "../canvas/Layer.hpp"
#include <iostream>

// Editor handles undo/redo and other editing operations
#include <algorithm>
#include <cstring>
#include <cstdio>

// HistoryState implementation
HistoryState::HistoryState(SDL_Texture* texture, int layerIndex)
    : m_texture(texture), m_layerIndex(layerIndex) {
}

HistoryState::~HistoryState() {
    if (m_texture) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }
}

HistoryState::HistoryState(HistoryState&& other) noexcept
    : m_texture(other.m_texture), m_layerIndex(other.m_layerIndex) {
    other.m_texture = nullptr;
}

HistoryState& HistoryState::operator=(HistoryState&& other) noexcept {
    if (this != &other) {
        if (m_texture) {
            SDL_DestroyTexture(m_texture);
        }
        m_texture = other.m_texture;
        m_layerIndex = other.m_layerIndex;
        other.m_texture = nullptr;
    }
    return *this;
}

// Editor implementation
// Singleton pattern - keeps track of edit history
Editor& Editor::getInstance() {
    static Editor instance;
    return instance;
}

Editor::Editor() = default;

void Editor::init() {
    // Load recent files from config
    loadRecentFiles();
}

void Editor::cleanup() {
    // Clear undo stack
    while (!m_undoStack.empty()) {
        m_undoStack.pop();
    }
    
    // Clear redo stack
    while (!m_redoStack.empty()) {
        m_redoStack.pop();
    }
}

void Editor::saveUndoState() {
    Canvas& canvas = Canvas::getInstance();
    int idx = canvas.getActiveLayerIndex();
    if (idx < 0 || idx >= static_cast<int>(canvas.getLayers().size())) return;
    
    Layer* activeLayer = canvas.getActiveLayer();
    if (!activeLayer) return;
    
    // Create a copy of the current layer's texture
    SDL_Texture* copy = SDL_CreateTexture(
        canvas.getRenderer(), 
        SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_TARGET, 
        canvas.getWidth(), 
        canvas.getHeight()
    );
    
    SDL_SetRenderTarget(canvas.getRenderer(), copy);
    SDL_RenderCopy(canvas.getRenderer(), activeLayer->getTexture(), nullptr, nullptr);
    SDL_SetRenderTarget(canvas.getRenderer(), nullptr);
    
    // Push onto undo stack
    m_undoStack.push(HistoryState(copy, idx));
    
    // HACK: Limit history size to prevent memory bloat
    limitHistorySize();
    
    // Clear redo stack when new action is performed
    while (!m_redoStack.empty()) {
        m_redoStack.pop();
    }
}

void Editor::limitHistorySize() {
    // Keep only the most recent MAX_HISTORY_SIZE states
    if (m_undoStack.size() > MAX_HISTORY_SIZE) {
        // Create a temporary stack to reverse the order
        std::stack<HistoryState> temp;
        
        // Move the most recent states to temp
        for (size_t i = 0; i < MAX_HISTORY_SIZE && !m_undoStack.empty(); i++) {
            temp.push(std::move(m_undoStack.top()));
            m_undoStack.pop();
        }
        
        // Clear remaining old states
        while (!m_undoStack.empty()) {
            m_undoStack.pop();
        }
        
        // Move states back to undo stack
        while (!temp.empty()) {
            m_undoStack.push(std::move(temp.top()));
            temp.pop();
        }
    }
}

void Editor::applyUndo() {
    if (m_undoStack.empty()) return;
    
    Canvas& canvas = Canvas::getInstance();
    Layer* activeLayer = canvas.getActiveLayer();
    if (!activeLayer) return;
    
    // Save current state for redo
    SDL_Texture* currentTexture = activeLayer->getTexture();
    SDL_Texture* copyTexture = SDL_CreateTexture(
        canvas.getRenderer(), 
        SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_TARGET, 
        canvas.getWidth(), 
        canvas.getHeight()
    );
    
    if (copyTexture) {
        SDL_SetRenderTarget(canvas.getRenderer(), copyTexture);
        SDL_RenderCopy(canvas.getRenderer(), currentTexture, nullptr, nullptr);
        SDL_SetRenderTarget(canvas.getRenderer(), nullptr);
        
        m_redoStack.push(HistoryState(copyTexture, canvas.getActiveLayerIndex()));
    }
    
    // Get the undo state
    HistoryState undoState = std::move(m_undoStack.top());
    m_undoStack.pop();
    
    // Apply the undo state
    canvas.setActiveLayerIndex(undoState.getLayerIndex());
    activeLayer = canvas.getActiveLayer();
    
    if (activeLayer) {
        // Transfer ownership of texture to layer
        activeLayer->setTexture(undoState.getTexture());
    }
}

void Editor::applyRedo() {
    if (m_redoStack.empty()) return;
    
    Canvas& canvas = Canvas::getInstance();
    Layer* activeLayer = canvas.getActiveLayer();
    if (!activeLayer) return;
    
    // Save current state for undo
    SDL_Texture* currentTexture = activeLayer->getTexture();
    SDL_Texture* copyTexture = SDL_CreateTexture(
        canvas.getRenderer(), 
        SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_TARGET, 
        canvas.getWidth(), 
        canvas.getHeight()
    );
    
    if (copyTexture) {
        SDL_SetRenderTarget(canvas.getRenderer(), copyTexture);
        SDL_RenderCopy(canvas.getRenderer(), currentTexture, nullptr, nullptr);
        SDL_SetRenderTarget(canvas.getRenderer(), nullptr);
        
        m_undoStack.push(HistoryState(copyTexture, canvas.getActiveLayerIndex()));
    }
    
    // Get the redo state
    HistoryState redoState = std::move(m_redoStack.top());
    m_redoStack.pop();
    
    // Apply the redo state
    canvas.setActiveLayerIndex(redoState.getLayerIndex());
    activeLayer = canvas.getActiveLayer();
    
    if (activeLayer) {
        // Transfer ownership of texture to layer
        activeLayer->setTexture(redoState.getTexture());
    }
}

void Editor::mergeLayers() {
    Canvas& canvas = Canvas::getInstance();
    if (canvas.getLayers().size() < 2) return;
    
    // Create a new layer for the merged result
    canvas.addLayer("Merged");
    Layer* mergedLayer = canvas.getActiveLayer();
    
    if (!mergedLayer) return;
    
    // Render all visible layers to the new layer
    SDL_SetRenderTarget(canvas.getRenderer(), mergedLayer->getTexture());
    
    for (const auto& layer : canvas.getLayers()) {
        if (layer->isVisible() && layer.get() != mergedLayer) {
            SDL_SetTextureAlphaMod(layer->getTexture(), static_cast<Uint8>(layer->getOpacity() * 255));
            SDL_RenderCopy(canvas.getRenderer(), layer->getTexture(), nullptr, nullptr);
        }
    }
    
    SDL_SetRenderTarget(canvas.getRenderer(), nullptr);
    
    // Remove all other layers
    // Note: This is inefficient but matches the original functionality
    for (int i = static_cast<int>(canvas.getLayers().size()) - 2; i >= 0; i--) {
        canvas.removeLayer(i);
    }
    
    canvas.setActiveLayerIndex(0);
}

void Editor::clearSelection() {
    Canvas& canvas = Canvas::getInstance();
    canvas.setHasSelection(false);
    canvas.setSelectionRect({0, 0, 0, 0});
    
    if (canvas.getSelectionTexture()) {
        SDL_DestroyTexture(canvas.getSelectionTexture());
        canvas.setSelectionTexture(nullptr);
    }
}

void Editor::copySelection() {
    Canvas& canvas = Canvas::getInstance();
    SDL_Rect selectionRect = canvas.getSelectionRect();
    
    if (!canvas.hasSelection() || selectionRect.w <= 0 || selectionRect.h <= 0) return;
    
    // Clean up existing selection texture
    if (canvas.getSelectionTexture()) {
        SDL_DestroyTexture(canvas.getSelectionTexture());
    }
    
    // Create new selection texture
    SDL_Texture* selectionTexture = SDL_CreateTexture(
        canvas.getRenderer(),
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        selectionRect.w,
        selectionRect.h
    );
    
    // Copy selected area to the texture
    SDL_SetRenderTarget(canvas.getRenderer(), selectionTexture);
    
    Layer* activeLayer = canvas.getActiveLayer();
    if (activeLayer) {
        SDL_RenderCopy(canvas.getRenderer(), activeLayer->getTexture(), &selectionRect, nullptr);
    }
    
    SDL_SetRenderTarget(canvas.getRenderer(), nullptr);
    
    canvas.setSelectionTexture(selectionTexture);
}

void Editor::pasteSelection() {
    Canvas& canvas = Canvas::getInstance();
    SDL_Texture* selectionTexture = canvas.getSelectionTexture();
    
    if (!selectionTexture) return;
    
    // Get dimensions of the selection texture
    int width, height;
    SDL_QueryTexture(selectionTexture, nullptr, nullptr, &width, &height);
    
    // Define paste position (offset from original)
    int pasteX = 10, pasteY = 10;
    SDL_Rect dest = {pasteX, pasteY, width, height};
    
    // Paste to active layer
    Layer* activeLayer = canvas.getActiveLayer();
    if (activeLayer) {
        SDL_SetRenderTarget(canvas.getRenderer(), activeLayer->getTexture());
        SDL_RenderCopy(canvas.getRenderer(), selectionTexture, nullptr, &dest);
        SDL_SetRenderTarget(canvas.getRenderer(), nullptr);
    }
    
    // Update selection rect
    canvas.setSelectionRect({pasteX, pasteY, width, height});
    canvas.setHasSelection(true);
}

void Editor::deleteSelection() {
    Canvas& canvas = Canvas::getInstance();
    SDL_Rect selectionRect = canvas.getSelectionRect();
    
    if (!canvas.hasSelection() || selectionRect.w <= 0 || selectionRect.h <= 0) return;
    
    Layer* activeLayer = canvas.getActiveLayer();
    if (!activeLayer) return;
    
    // Clear the selected area on the active layer
    SDL_SetRenderTarget(canvas.getRenderer(), activeLayer->getTexture());
    SDL_SetRenderDrawBlendMode(canvas.getRenderer(), SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(canvas.getRenderer(), 0, 0, 0, 0);
    SDL_RenderFillRect(canvas.getRenderer(), &selectionRect);
    SDL_SetRenderTarget(canvas.getRenderer(), nullptr);
    
    clearSelection();
}

// Recent Files implementation
void Editor::addRecentFile(const std::string& filepath) {
    // Remove if already exists to avoid duplicates
    auto it = std::find(m_recentFiles.begin(), m_recentFiles.end(), filepath);
    if (it != m_recentFiles.end()) {
        m_recentFiles.erase(it);
    }
    
    // Add to front of list
    m_recentFiles.insert(m_recentFiles.begin(), filepath);
    
    // Limit to MAX_RECENT_FILES
    if (m_recentFiles.size() > MAX_RECENT_FILES) {
        m_recentFiles.resize(MAX_RECENT_FILES);
    }
    
    // Auto-save to config file
    saveRecentFiles();
}

void Editor::clearRecentFiles() {
    m_recentFiles.clear();
    saveRecentFiles();
}

void Editor::loadRecentFiles() {
    // Simple file-based storage - create .enough_recent_files in user directory
    // TODO: Use proper config directory based on platform
    std::string configFile = ".enough_recent_files";
    
    FILE* file = fopen(configFile.c_str(), "r");
    if (!file) return;
    
    char line[512];
    m_recentFiles.clear();
    
    while (fgets(line, sizeof(line), file) && m_recentFiles.size() < MAX_RECENT_FILES) {
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        // Skip empty lines
        if (strlen(line) > 0) {
            m_recentFiles.push_back(std::string(line));
        }
    }
    
    fclose(file);
}

void Editor::saveRecentFiles() {
    // Save to simple text file
    std::string configFile = ".enough_recent_files";
    
    FILE* file = fopen(configFile.c_str(), "w");
    if (!file) return;
    
    for (const auto& filepath : m_recentFiles) {
        fprintf(file, "%s\n", filepath.c_str());
    }
    
    fclose(file);
}