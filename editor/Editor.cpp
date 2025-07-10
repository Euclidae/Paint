#include "Editor.hpp"
#include "../canvas/Canvas.hpp"
#include "../canvas/Layer.hpp"
#include <iostream>

#include <algorithm>
#include <cstring>
#include <cstdio>

// To be honest, I do not think we need any comments here as everything should be straight forward.
// history will just help us with stuff like ctrl + z. Just like chrome tabs you push and pop... wait, that's a queue (the DSA for move back n forth)
// Regardess, pretty simple.
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

Editor& Editor::getInstance() {
    static Editor instance;
    return instance;
}

Editor::Editor() = default;

void Editor::init() {
    loadRecentFiles();
}

void Editor::cleanup() {
    while (!m_undoStack.empty()) {
        m_undoStack.pop();
    }
    
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
    
    m_undoStack.push(HistoryState(copy, idx));
    

    limitHistorySize();
    
    while (!m_redoStack.empty()) {
        m_redoStack.pop();
    }
}

void Editor::limitHistorySize() {
    if (m_undoStack.size() > MAX_HISTORY_SIZE) {
        std::stack<HistoryState> temp;
        
        for (size_t i = 0; i < MAX_HISTORY_SIZE && !m_undoStack.empty(); i++) {
            temp.push(std::move(m_undoStack.top()));
            m_undoStack.pop();
        }
        
        while (!m_undoStack.empty()) {
            m_undoStack.pop();
        }
        
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
    
    HistoryState undoState = std::move(m_undoStack.top());
    m_undoStack.pop();
    
    canvas.setActiveLayerIndex(undoState.getLayerIndex());
    activeLayer = canvas.getActiveLayer();
    
    if (activeLayer) {
        activeLayer->setTexture(undoState.getTexture());
    }
}

void Editor::applyRedo() {
    if (m_redoStack.empty()) return;
    
    Canvas& canvas = Canvas::getInstance();
    Layer* activeLayer = canvas.getActiveLayer();
    if (!activeLayer) return;
    
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
    
    HistoryState redoState = std::move(m_redoStack.top());
    m_redoStack.pop();
    
    canvas.setActiveLayerIndex(redoState.getLayerIndex());
    activeLayer = canvas.getActiveLayer();
    
    if (activeLayer) {
        activeLayer->setTexture(redoState.getTexture());
    }
}

void Editor::mergeLayers() {
    Canvas& canvas = Canvas::getInstance();
    if (canvas.getLayers().size() < 2) return;
    
    canvas.addLayer("Merged");
    Layer* mergedLayer = canvas.getActiveLayer();
    
    if (!mergedLayer) return;
    
    SDL_SetRenderTarget(canvas.getRenderer(), mergedLayer->getTexture());
    
    for (const auto& layer : canvas.getLayers()) {
        if (layer->isVisible() && layer.get() != mergedLayer) {
            SDL_SetTextureAlphaMod(layer->getTexture(), static_cast<Uint8>(layer->getOpacity() * 255));
            SDL_RenderCopy(canvas.getRenderer(), layer->getTexture(), nullptr, nullptr);
        }
    }
    
    SDL_SetRenderTarget(canvas.getRenderer(), nullptr);
    

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
    
    if (canvas.getSelectionTexture()) {
        SDL_DestroyTexture(canvas.getSelectionTexture());
    }
    
    SDL_Texture* selectionTexture = SDL_CreateTexture(
        canvas.getRenderer(),
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        selectionRect.w,
        selectionRect.h
    );
    
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
    
    int width, height;
    SDL_QueryTexture(selectionTexture, nullptr, nullptr, &width, &height);
    
    int pasteX = 10, pasteY = 10;
    SDL_Rect dest = {pasteX, pasteY, width, height};
    
    Layer* activeLayer = canvas.getActiveLayer();
    if (activeLayer) {
        SDL_SetRenderTarget(canvas.getRenderer(), activeLayer->getTexture());
        SDL_RenderCopy(canvas.getRenderer(), selectionTexture, nullptr, &dest);
        SDL_SetRenderTarget(canvas.getRenderer(), nullptr);
    }
    
    canvas.setSelectionRect({pasteX, pasteY, width, height});
    canvas.setHasSelection(true);
}

void Editor::deleteSelection() {
    Canvas& canvas = Canvas::getInstance();
    SDL_Rect selectionRect = canvas.getSelectionRect();
    
    if (!canvas.hasSelection() || selectionRect.w <= 0 || selectionRect.h <= 0) return;
    
    Layer* activeLayer = canvas.getActiveLayer();
    if (!activeLayer) return;
    
    SDL_SetRenderTarget(canvas.getRenderer(), activeLayer->getTexture());
    SDL_SetRenderDrawBlendMode(canvas.getRenderer(), SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(canvas.getRenderer(), 0, 0, 0, 0);
    SDL_RenderFillRect(canvas.getRenderer(), &selectionRect);
    SDL_SetRenderTarget(canvas.getRenderer(), nullptr);
    
    clearSelection();
}

void Editor::addRecentFile(const std::string& filepath) {
    auto it = std::find(m_recentFiles.begin(), m_recentFiles.end(), filepath);
    if (it != m_recentFiles.end()) {
        m_recentFiles.erase(it);
    }
    
    m_recentFiles.insert(m_recentFiles.begin(), filepath);
    
    if (m_recentFiles.size() > MAX_RECENT_FILES) {
        m_recentFiles.resize(MAX_RECENT_FILES);
    }
    
    saveRecentFiles();
}

void Editor::clearRecentFiles() {
    m_recentFiles.clear();
    saveRecentFiles();
}

void Editor::loadRecentFiles() {

    std::string configFile = ".enough_recent_files";
    
    FILE* file = fopen(configFile.c_str(), "r");
    if (!file) return;
    
    char line[512];
    m_recentFiles.clear();
    
    while (fgets(line, sizeof(line), file) && m_recentFiles.size() < MAX_RECENT_FILES) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        if (strlen(line) > 0) {
            m_recentFiles.push_back(std::string(line));
        }
    }
    
    fclose(file);
}

void Editor::saveRecentFiles() {
    std::string configFile = ".enough_recent_files";
    
    FILE* file = fopen(configFile.c_str(), "w");
    if (!file) return;
    
    for (const auto& filepath : m_recentFiles) {
        fprintf(file, "%s\n", filepath.c_str());
    }
    
    fclose(file);
}