#pragma once
#include <SDL2/SDL.h>
#include <stack>
#include <memory>
#include <vector>
#include <string>

class Canvas;

class HistoryState {
public:
    HistoryState(SDL_Texture* texture, int layerIndex);
    ~HistoryState();
    

    HistoryState(const HistoryState&) = delete;
    HistoryState& operator=(const HistoryState&) = delete;
    

    HistoryState(HistoryState&& other) noexcept;
    HistoryState& operator=(HistoryState&& other) noexcept;
    
    SDL_Texture* getTexture() const { return m_texture; }
    int getLayerIndex() const { return m_layerIndex; }
    
private:
    SDL_Texture* m_texture = nullptr;
    int m_layerIndex = -1;
};

class Editor {
public:
    static Editor& getInstance();
    
    void init();
    void cleanup();
    

    void saveUndoState();
    void applyUndo();
    void applyRedo();
    size_t getUndoStackSize() const { return m_undoStack.size(); }
    size_t getRedoStackSize() const { return m_redoStack.size(); }
    

    void limitHistorySize();
    

    void save_undo_state() { saveUndoState(); }
    void apply_undo() { applyUndo(); }
    

    void mergeLayers();
    

    void clearSelection();
    void copySelection();
    void pasteSelection();
    void deleteSelection();
    

    void addRecentFile(const std::string& filepath);
    const std::vector<std::string>& getRecentFiles() const { return m_recentFiles; }
    void clearRecentFiles();
    void loadRecentFiles();
    void saveRecentFiles();
    
private:
    Editor();
    ~Editor() = default;
    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;
    
    std::stack<HistoryState> m_undoStack;
    std::stack<HistoryState> m_redoStack;
    

    std::vector<std::string> m_recentFiles;
    

    static const size_t MAX_HISTORY_SIZE = 50;
    static const size_t MAX_RECENT_FILES = 10;
};


inline Editor& GetEditor() {
    return Editor::getInstance();
}