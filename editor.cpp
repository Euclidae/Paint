#include "editor.hpp"
#include "canvas.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>
// Phased out TUI mostly. Kept the grey as a border around IMGUI. Might apply a theme later.
extern SDL_Rect selectionRect;
extern bool hasSelection;
extern SDL_Texture* selectionTexture;
extern ImVec2 selectionOffset;

std::stack<HistoryState> Editor::undoStack;
std::stack<HistoryState> Editor::redoStack;

void Editor::init() {}

void Editor::cleanup() {
    while (!undoStack.empty()) {
        SDL_DestroyTexture(undoStack.top().texture);
        undoStack.pop();
    }
    while (!redoStack.empty()) {
        SDL_DestroyTexture(redoStack.top().texture);
        redoStack.pop();
    }
}

// NB, it might be worth looking into implementing a state machine for undo/redo functionality.
void Editor::saveUndoState() {
    using namespace Canvas;
    int idx = activeLayerIndex;
    if (idx < 0 || idx >= static_cast<int>(layers.size())) return;

    SDL_Texture* copy = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, CANVAS_WIDTH, CANVAS_HEIGHT);
    SDL_SetRenderTarget(renderer, copy);
    SDL_RenderCopy(renderer, layers[idx].texture, nullptr, nullptr);
    SDL_SetRenderTarget(renderer, nullptr);
    HistoryState state{copy, idx};
    undoStack.push(state);
    while (!redoStack.empty()) {
        SDL_DestroyTexture(redoStack.top().texture);
        redoStack.pop();
    }
}

void Editor::applyUndo() {
    using namespace Canvas;
    if (undoStack.empty()) return;
    HistoryState currentState{layers[undoStack.top().layerIndex].texture, undoStack.top().layerIndex};
    redoStack.push(currentState);
    HistoryState undoState = undoStack.top(); undoStack.pop();
    activeLayerIndex = undoState.layerIndex;
    layers[activeLayerIndex].texture = undoState.texture;
}

void Editor::applyRedo() {
    using namespace Canvas;
    if (redoStack.empty()) return;
    HistoryState currentState{layers[redoStack.top().layerIndex].texture, redoStack.top().layerIndex};
    undoStack.push(currentState);
    HistoryState redoState = redoStack.top(); redoStack.pop();
    activeLayerIndex = redoState.layerIndex;
    layers[activeLayerIndex].texture = redoState.texture;
}

void Editor::mergeLayers() {
    using namespace Canvas;
    if (layers.size() < 2) return;
    addLayer("Merged");
    SDL_SetRenderTarget(renderer, layers[activeLayerIndex].texture);
    for (const auto& layer : layers) {
        if (layer.visible && layer.texture != layers[activeLayerIndex].texture) {
            SDL_SetTextureAlphaMod(layer.texture, (Uint8)(layer.opacity * 255));
            SDL_RenderCopy(renderer, layer.texture, nullptr, nullptr);
        }
    }
    SDL_SetRenderTarget(renderer, nullptr);
    for (int i = static_cast<int>(layers.size()) - 2; i >= 0; i--) {
        if (layers[i].texture) SDL_DestroyTexture(layers[i].texture);
        layers.erase(layers.begin() + i);
    }
    activeLayerIndex = 0;
}

void Editor::clearSelection() {
    hasSelection = false;
    selectionRect = {0, 0, 0, 0};
    if (selectionTexture) {
        SDL_DestroyTexture(selectionTexture);
        selectionTexture = nullptr;
    }
}

void Editor::copySelection() {
    using namespace Canvas;
    if (!hasSelection || selectionRect.w <= 0 || selectionRect.h <= 0) return;
    if (selectionTexture) SDL_DestroyTexture(selectionTexture);
    selectionTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, selectionRect.w, selectionRect.h);
    SDL_SetRenderTarget(renderer, selectionTexture);
    SDL_RenderCopy(renderer, layers[activeLayerIndex].texture, &selectionRect, nullptr);
    SDL_SetRenderTarget(renderer, nullptr);
}

void Editor::pasteSelection() {
    using namespace Canvas;
    if (!selectionTexture) return;

    int pasteX = 10, pasteY = 10;
    SDL_Rect dest = {pasteX, pasteY, selectionRect.w, selectionRect.h};
    SDL_SetRenderTarget(renderer, layers[activeLayerIndex].texture);
    SDL_RenderCopy(renderer, selectionTexture, nullptr, &dest);
    SDL_SetRenderTarget(renderer, nullptr);

    selectionRect.x = pasteX;
    selectionRect.y = pasteY;
    hasSelection = true;
}

void Editor::deleteSelection() {
    using namespace Canvas;
    if (!hasSelection || selectionRect.w <= 0 || selectionRect.h <= 0) return;
    SDL_SetRenderTarget(renderer, layers[activeLayerIndex].texture);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_Rect rect = selectionRect;
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderTarget(renderer, nullptr);
    clearSelection();
}

// Some snake_case methods for variety
void Editor::save_undo_state() {
    saveUndoState();
}

void Editor::apply_undo() {
    applyUndo();
}

size_t Editor::getUndoStackSize() {
    return undoStack.size();
}
