// I always forget if Windows needs this or not... better safe than sorry
// Update windows needs this apparently because SDL on windows runs its own main function and as such the macro here tells sdl we
// created the main function so dont run SDLmain
#ifdef _WIN32
#define SDL_MAIN_HANDLED 1
#endif

#include "Paint.hpp"

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    if (!Paint::Initialize()) {
        return 1;
    }

    if (!Paint::CreateWindowAndRenderer(&window, &renderer, "Enough Image Editor")) {
        Paint::Cleanup();
        return 1;
    }

    if (!Paint::InitializeImGui(window, renderer)) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Paint::Cleanup();
        return 1;
    }

    Canvas& canvas = GetCanvas();
    canvas.init(renderer);
    GetToolManager().init();
    GetEditor().init();
    //GetUI().init(window, renderer); Caused double initialization. THat is now taken care of in the UI singleton instead in paint.

    bool quit = false;
    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            SDL_Point mousePos = {event.button.x, event.button.y};

            if (!ImGui::GetIO().WantCaptureMouse) {
                if (canvas.handleResizeEvent(event, mousePos)) {
                    continue;
                }
                GetToolManager().handleSDLEvent(event);
            }

            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }

        // Start new ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Draw everything
        GetUI().render();
        canvas.render();

        // Render ImGui stuff
        ImGui::Render();
        SDL_RenderSetScale(renderer, ImGui::GetIO().DisplayFramebufferScale.x, ImGui::GetIO().DisplayFramebufferScale.y);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());

        // Show the frame
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // cap at roughly 60fps
    }

    // Clean up our mess
    Paint::CleanupImGui();

    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);

    Paint::Cleanup();

    return 0;
}
