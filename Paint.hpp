#pragma once

// Unified header for the Enough Image Editor
// Include this to get access to all the main components
// Saves having to include each header separately

#include "canvas/Canvas.hpp"
#include "canvas/Layer.hpp"
#include "tools/Tool.hpp"
#include "editor/Editor.hpp"
#include "ui/UI.hpp"

// External library headers - SDL for graphics, ImGui for UI
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"


// Standard library headers
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <cmath>


namespace Paint {
    inline bool Initialize() {
        if (SDL_Init(SDL_INIT_VIDEO)) {
            std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
            return false;
        }

        int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            std::cerr << "SDL_image init failed: " << IMG_GetError() << std::endl;
            return false;
        }

        if (TTF_Init() == -1) {
            std::cerr << "SDL_ttf init failed: " << TTF_GetError() << std::endl;
            return false;
        }

        return true;
    }

    inline void Cleanup() {
        GetUI().cleanup();
        GetEditor().cleanup();
        GetToolManager().cleanup();
        GetCanvas().cleanup();

        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
    }

    inline bool CreateWindowAndRenderer(SDL_Window** window, SDL_Renderer** renderer, const char* title = "Paint") {
        Canvas& canvas = GetCanvas();

        *window = SDL_CreateWindow(
            title,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            canvas.getWidth() + 300,
            canvas.getHeight(),
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );

        if (!*window) {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            return false;
        }

        *renderer = SDL_CreateRenderer(
            *window,
            -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
        );

        if (!*renderer) {
            std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
            return false;
        }

        return true;
    }

    inline bool InitializeImGui(SDL_Window* window, SDL_Renderer* renderer) {
        return UI::getInstance().init(window, renderer);
    }

    inline void CleanupImGui() {
        // ImGui_ImplSDLRenderer2_Shutdown();
        // ImGui_ImplSDL2_Shutdown();
        // ImGui::DestroyContext();
        UI::getInstance().cleanup();
    }
}
