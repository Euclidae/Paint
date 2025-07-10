#include "Layer.hpp"
#include <SDL2/SDL_image.h>
#include <iostream>

Layer::Layer(const std::string& name) 
    : m_name(name), m_opacity(1.0f), m_visible(true), m_locked(false),
      m_blendMode(0), m_selected(false), m_beingDragged(false), m_useMask(false),
      m_x(0), m_y(0), m_maskDirty(false) {
}

Layer::~Layer() {
    cleanup();
}

Layer::Layer(Layer&& other) noexcept
    : m_texture(other.m_texture),
      m_name(std::move(other.m_name)),
      m_opacity(other.m_opacity),
      m_visible(other.m_visible),
      m_locked(other.m_locked),
      m_blendMode(other.m_blendMode),
      m_selected(other.m_selected),
      m_beingDragged(other.m_beingDragged),
      m_mask(other.m_mask),
      m_useMask(other.m_useMask),
      m_x(other.m_x),
      m_y(other.m_y),
      m_maskDirty(other.m_maskDirty) {
    
    // Reset the moved-from object
    other.m_texture = nullptr;
    other.m_mask = nullptr;
    other.m_x = 0;
    other.m_y = 0;
}

Layer& Layer::operator=(Layer&& other) noexcept {
    if (this != &other) {
        // Clean up existing resources
        cleanup();
        
        // Transfer ownership
        m_texture = other.m_texture;
        m_name = std::move(other.m_name);
        m_opacity = other.m_opacity;
        m_visible = other.m_visible;
        m_locked = other.m_locked;
        m_blendMode = other.m_blendMode;
        m_selected = other.m_selected;
        m_beingDragged = other.m_beingDragged;
        m_mask = other.m_mask;
        m_useMask = other.m_useMask;
        m_x = other.m_x;
        m_y = other.m_y;
        m_maskDirty = other.m_maskDirty;
        
        // Reset the moved-from object
        other.m_texture = nullptr;
        other.m_mask = nullptr;
        other.m_x = 0;
        other.m_y = 0;
        other.m_maskDirty = false;
    }
    return *this;
}

void Layer::setTexture(SDL_Texture* texture) {
    if (m_texture) {
        SDL_DestroyTexture(m_texture);
    }
    m_texture = texture;
}

void Layer::setMask(SDL_Texture* mask) {
    if (m_mask) {
        SDL_DestroyTexture(m_mask);
    }
    m_mask = mask;
    m_maskDirty = true; // Mark mask as needing updates
}

void Layer::duplicate(Layer& newLayer) const {
    newLayer.m_name = m_name + " Copy";
    newLayer.m_opacity = m_opacity;
    newLayer.m_visible = m_visible;
    newLayer.m_locked = m_locked;
    newLayer.m_blendMode = m_blendMode;
    newLayer.m_selected = false;
    newLayer.m_beingDragged = false;
    newLayer.m_useMask = m_useMask;
    newLayer.m_x = m_x;
    newLayer.m_y = m_y;
    newLayer.m_maskDirty = false; // New layer starts clean
    
    // We don't copy the texture or mask here because they should be
    // duplicated at a higher level where the renderer is available
}

void Layer::clear(SDL_Renderer* renderer) {
    if (!m_texture) return;
    
    // Get texture dimensions
    int width, height;
    SDL_QueryTexture(m_texture, nullptr, nullptr, &width, &height);
    
    // Create a new target for rendering
    SDL_Texture* originalTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, m_texture);
    
    // Clear the texture with transparency
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    
    // Restore the original render target
    SDL_SetRenderTarget(renderer, originalTarget);
}

void Layer::createEmptyMask(SDL_Renderer* renderer, int width, int height) {
    // Clean up existing mask first
    if (m_mask) {
        SDL_DestroyTexture(m_mask);
    }
    
    // Create new mask texture - white means fully visible
    m_mask = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, 
                               SDL_TEXTUREACCESS_TARGET, width, height);
    
    if (!m_mask) {
        std::cerr << "Failed to create mask texture: " << SDL_GetError() << std::endl;
        return;
    }
    
    // Set up the mask for transparency blending
    SDL_SetTextureBlendMode(m_mask, SDL_BLENDMODE_BLEND);
    
    // Fill with white (fully opaque) - this is the "reveal all" default
    SDL_Texture* originalTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, m_mask);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White = show everything
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, originalTarget);
    
    m_maskDirty = true;
}

void Layer::clearMask(SDL_Renderer* renderer) {
    if (!m_mask) return;
    
    // Clear to white (show everything) - this is non-destructive
    SDL_Texture* originalTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, m_mask);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, originalTarget);
    
    m_maskDirty = true;
}

void Layer::invertMask(SDL_Renderer* renderer) {
    if (!m_mask) return;
    
    // Get mask dimensions
    int width, height;
    SDL_QueryTexture(m_mask, nullptr, nullptr, &width, &height);
    
    // Read current mask pixels
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    
    if (!surface) return;
    
    SDL_Texture* originalTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, m_mask);
    
    // HACK: Not sure why SDL needs this but it prevents weird artifacts
    if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, 
                           surface->pixels, surface->pitch) != 0) {
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(renderer, originalTarget);
        return;
    }
    
    // Invert the mask pixels
    SDL_LockSurface(surface);
    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    
    for (int i = 0; i < surface->w * surface->h; i++) {
        Uint8 r, g, b, a;
        SDL_GetRGBA(pixels[i], surface->format, &r, &g, &b, &a);
        
        // Invert RGB channels but keep alpha
        r = 255 - r;
        g = 255 - g;
        b = 255 - b;
        
        pixels[i] = SDL_MapRGBA(surface->format, r, g, b, a);
    }
    
    SDL_UnlockSurface(surface);
    
    // Create new texture from inverted surface
    SDL_Texture* newMask = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    
    if (newMask) {
        SDL_SetTextureBlendMode(newMask, SDL_BLENDMODE_BLEND);
        SDL_DestroyTexture(m_mask);
        m_mask = newMask;
        m_maskDirty = true;
    }
    
    SDL_SetRenderTarget(renderer, originalTarget);
}

void Layer::applyMaskToTexture(SDL_Renderer* renderer) {
    if (!m_mask || !m_texture) return;
    
    // Get texture dimensions
    int width, height;
    SDL_QueryTexture(m_texture, nullptr, nullptr, &width, &height);
    
    // Create a temporary texture for the result
    SDL_Texture* tempTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                                SDL_TEXTUREACCESS_TARGET, width, height);
    
    if (!tempTexture) return;
    
    SDL_Texture* originalTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, tempTexture);
    
    // Clear the temp texture
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    
    // TODO: This is a simplified mask application - could be more sophisticated
    // First render the original texture
    SDL_RenderCopy(renderer, m_texture, nullptr, nullptr);
    
    // Then apply the mask using multiply blend mode
    SDL_SetTextureBlendMode(m_mask, SDL_BLENDMODE_MOD);
    SDL_RenderCopy(renderer, m_mask, nullptr, nullptr);
    
    SDL_SetRenderTarget(renderer, originalTarget);
    
    // Replace the original texture with the masked version
    SDL_DestroyTexture(m_texture);
    m_texture = tempTexture;
    
    // Clean up the mask since it's been applied
    SDL_DestroyTexture(m_mask);
    m_mask = nullptr;
    m_useMask = false;
    m_maskDirty = false;
}

void Layer::renderWithMask(SDL_Renderer* renderer, SDL_Rect destRect, float globalOpacity) {
    if (!m_texture || !m_visible) return;
    
    // Calculate final opacity
    float finalOpacity = m_opacity * globalOpacity;
    Uint8 alpha = static_cast<Uint8>(finalOpacity * 255);
    
    if (!m_mask || !m_useMask) {
        // Simple render without mask
        SDL_SetTextureAlphaMod(m_texture, alpha);
        SDL_RenderCopy(renderer, m_texture, nullptr, &destRect);
        return;
    }
    
    // PERF: Could optimize this by caching the composited result
    // But for now, just render with mask each time
    
    // Create temporary texture for masked rendering
    SDL_Texture* tempTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                                SDL_TEXTUREACCESS_TARGET, 
                                                destRect.w, destRect.h);
    
    if (!tempTexture) {
        // Fallback to rendering without mask
        SDL_SetTextureAlphaMod(m_texture, alpha);
        SDL_RenderCopy(renderer, m_texture, nullptr, &destRect);
        return;
    }
    
    SDL_Texture* originalTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, tempTexture);
    
    // Clear temp texture
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    
    // Render the layer content
    SDL_Rect fullRect = {0, 0, destRect.w, destRect.h};
    SDL_RenderCopy(renderer, m_texture, nullptr, &fullRect);
    
    // Apply mask
    SDL_SetTextureBlendMode(m_mask, SDL_BLENDMODE_MOD);
    SDL_RenderCopy(renderer, m_mask, nullptr, &fullRect);
    
    // Restore original render target
    SDL_SetRenderTarget(renderer, originalTarget);
    
    // Render the masked result with opacity
    SDL_SetTextureAlphaMod(tempTexture, alpha);
    SDL_RenderCopy(renderer, tempTexture, nullptr, &destRect);
    
    // Clean up
    SDL_DestroyTexture(tempTexture);
}

void Layer::cleanup() {
    if (m_texture) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }
    
    if (m_mask) {
        SDL_DestroyTexture(m_mask);
        m_mask = nullptr;
    }
}