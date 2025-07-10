#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <memory>

class Layer {
public:
    Layer(const std::string& name = "New Layer");
    ~Layer();
    
    Layer(const Layer& other) = delete;
    Layer& operator=(const Layer& other) = delete;
    
    Layer(Layer&& other) noexcept;
    Layer& operator=(Layer&& other) noexcept;
    
    SDL_Texture* getTexture() const { return m_texture; }
    void setTexture(SDL_Texture* texture);
    
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    float getOpacity() const { return m_opacity; }
    void setOpacity(float opacity) { m_opacity = opacity; }
    
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    
    bool isLocked() const { return m_locked; }
    void setLocked(bool locked) { m_locked = locked; }
    
    int getBlendMode() const { return m_blendMode; }
    void setBlendMode(int mode) { m_blendMode = mode; }
    
    bool isSelected() const { return m_selected; }
    void setSelected(bool selected) { m_selected = selected; }
    
    bool isBeingDragged() const { return m_beingDragged; }
    void setBeingDragged(bool dragged) { m_beingDragged = dragged; }
    
    SDL_Texture* getMask() const { return m_mask; }
    void setMask(SDL_Texture* mask);
    
    bool isUsingMask() const { return m_useMask; }
    void setUseMask(bool use) { m_useMask = use; }
    bool hasMask() const { return m_mask != nullptr; }
    
    // Layer position for moving content without texture recreation
    int getX() const { return m_x; }
    int getY() const { return m_y; }
    void setX(int x) { m_x = x; }
    void setY(int y) { m_y = y; }
    void setPosition(int x, int y) { m_x = x; m_y = y; }
    void moveBy(int dx, int dy) { m_x += dx; m_y += dy; }
    
    void createEmptyMask(SDL_Renderer* renderer, int width, int height);
    void clearMask(SDL_Renderer* renderer);
    void invertMask(SDL_Renderer* renderer);// 
    
    void duplicate(Layer& newLayer) const;
    void clear(SDL_Renderer* renderer);
    
    // TODO: Add layer blending modes beyond basic alpha
    // PERF: Could cache composited result when mask doesn't change
    void renderWithMask(SDL_Renderer* renderer, SDL_Rect destRect, float globalOpacity = 1.0f);
    
private:
    SDL_Texture* m_texture = nullptr;
    std::string m_name;
    float m_opacity = 1.0f;
    bool m_visible = true;
    bool m_locked = false;
    int m_blendMode = 0;
    bool m_selected = false;
    bool m_beingDragged = false;
    SDL_Texture* m_mask = nullptr;
    bool m_useMask = false;
    
    int m_x = 0;
    int m_y = 0;
    
    // This is a hack. Track if mask was modified for optimization
    bool m_maskDirty = false;
    
    void cleanup();
};