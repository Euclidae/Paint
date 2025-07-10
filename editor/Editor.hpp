#pragma once
#include <SDL2/SDL.h>
#include <stack>
#include <memory>
#include <vector>
#include <string>

class Canvas;

// History state class for undo/redo operations
class HistoryState {
public:
    HistoryState(SDL_Texture* texture, int layerIndex);
    ~HistoryState();
    
    // No copying allowed
    HistoryState(const HistoryState&) = delete;
    HistoryState& operator=(const HistoryState&) = delete;
    
    // Move semantics
    HistoryState(HistoryState&& other) noexcept;
    HistoryState& operator=(HistoryState&& other) noexcept;
    
    SDL_Texture* getTexture() const { return m_texture; }
    int getLayerIndex() const { return m_layerIndex; }
    
private:
    SDL_Texture* m_texture = nullptr;
    int m_layerIndex = -1;
};

// Editor class for managing undo/redo and selections
class Editor {
public:
    static Editor& getInstance();
    
    void init();
    void cleanup();
    
    // Undo/Redo functionality - improved for multiple operations
    void saveUndoState();
    void applyUndo();
    void applyRedo();
    size_t getUndoStackSize() const { return m_undoStack.size(); }
    size_t getRedoStackSize() const { return m_redoStack.size(); }
    
    // HACK: Clear history when it gets too big - prevents memory bloat
    void limitHistorySize();
    
    // For compatibility with the old code
    void save_undo_state() { saveUndoState(); }
    void apply_undo() { applyUndo(); }
    
    // Layer operations
    void mergeLayers();
    
    // Selection operations
    void clearSelection();
    void copySelection();
    void pasteSelection();
    void deleteSelection();
    
    // Recent files functionality
    void addRecentFile(const std::string& filepath);
    const std::vector<std::string>& getRecentFiles() const { return m_recentFiles; }
    void clearRecentFiles();
    void loadRecentFiles(); // Load from config file
    void saveRecentFiles(); // Save to config file
    
private:
    Editor();
    ~Editor() = default;
    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;
    
    std::stack<HistoryState> m_undoStack;
    std::stack<HistoryState> m_redoStack;
    
    // Recent files storage
    std::vector<std::string> m_recentFiles;
    
    // HACK: Limit history to prevent memory issues - 50 seems reasonable
    static const size_t MAX_HISTORY_SIZE = 50;
    static const size_t MAX_RECENT_FILES = 10;
};

// Helper function to get the editor instance
inline Editor& GetEditor() {
    return Editor::getInstance();
}