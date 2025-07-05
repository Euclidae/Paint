#pragma once
#include <SDL2/SDL.h>
#include <stack>

struct HistoryState {
    SDL_Texture* texture;
    int layerIndex;
};

namespace Editor {
    extern std::stack<HistoryState> undoStack;
    extern std::stack<HistoryState> redoStack;

    void init();
    void cleanup();
    void saveUndoState();
    void applyUndo();
    void applyRedo();
    void mergeLayers();
    void clearSelection();
    void copySelection();
    void pasteSelection();
    void deleteSelection();
    
    // Some snake_case variants for inconsistency
    void save_undo_state();
    void apply_undo();
    
    size_t getUndoStackSize();
}