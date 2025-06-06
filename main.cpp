#ifdef _WIN32
    #define SDL_MAIN_HANDLED
#endif

//N.B should probably add blue...
//update added blue and then some.
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <functional>
#include <iostream>
#include <stack>
#include <algorithm>
#include <string>


const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int TUI_WIDTH = 200; // Width of the TUI panel. Can't figure out how to use imgui so I am gonna do this. Will deal with ImGui some other time.
const int CANVAS_WIDTH = WINDOW_WIDTH - TUI_WIDTH;

// Tool states
bool pencilTool = false, eraserTool = false, lineTool = false, paintTool = false, rectTool = false;
bool isDrawing = false, isErasing = false, isLining = false, isDrawingRect = false;

// Current color (RGB 0-255)
Uint8 currColor[3] = { 0, 0, 0 }; // Default black
std::vector<std::pair<int, int>> points;
std::vector<std::function<void(SDL_Renderer*)>> functionsToCall;
std::vector<std::pair<int, int>> eraserCurves;
std::vector<std::pair<int, int>> lineCurves(2);
std::vector<std::pair<int, int>> lineCurves1(2);
std::vector<std::pair<int, int>> paintedPoints;
std::vector<std::pair<int, int>> rectPoints;

Uint32 targetColor;

// SDL objects
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Surface* surface = nullptr;
TTF_Font* font = nullptr;

void floodFillStack(int x, int y, Uint8 fillColor[3], Uint32 targetCol) {
    std::stack<std::pair<int, int>> pointstoProcess;
    pointstoProcess.push(std::make_pair(x, y));

    bool visited[CANVAS_WIDTH][WINDOW_HEIGHT];
    std::fill(&visited[0][0], &visited[0][0] + sizeof(visited), false);

    // Get surface for pixel reading
    SDL_Surface* windowSurface = SDL_GetWindowSurface(window);
    if (!windowSurface) return;

    while (!pointstoProcess.empty()) {
        std::pair<int, int> current = pointstoProcess.top();
        pointstoProcess.pop();
        x = current.first;
        y = current.second;

        if (x < 0 || x >= CANVAS_WIDTH || y < 0 || y >= WINDOW_HEIGHT || visited[x][y]) {
            continue;
        }
        visited[x][y] = true;

        // Read pixel color from surface
        Uint32* pixels = (Uint32*)windowSurface->pixels;
        Uint32 pixelColor = pixels[y * CANVAS_WIDTH + x];

        if (pixelColor == targetCol) {
            paintedPoints.push_back(std::make_pair(x, y));

            pointstoProcess.push(std::make_pair(x + 1, y));
            pointstoProcess.push(std::make_pair(x, y + 1));
            pointstoProcess.push(std::make_pair(x - 1, y));
            pointstoProcess.push(std::make_pair(x, y - 1));
        }
    }
}

void drawLineRealtime(SDL_Renderer* rend) {
    SDL_SetRenderDrawColor(rend, currColor[0], currColor[1], currColor[2], 255);

    if (points.size() > 1) {
        for (size_t i = 1; i < points.size(); ++i) {
            SDL_RenderDrawLine(rend, points[i - 1].first, points[i - 1].second,
                points[i].first, points[i].second);
        }
    }
    else if (points.size() == 1) {
        SDL_RenderDrawPoint(rend, points[0].first, points[0].second);
    }
}

void drawLine(SDL_Renderer* rend, const std::vector<std::pair<int, int>>& tempPoints, const Uint8* tempColor) {
    SDL_SetRenderDrawColor(rend, tempColor[0], tempColor[1], tempColor[2], 255);

    if (tempPoints.size() > 1) {
        for (size_t i = 1; i < tempPoints.size(); ++i) {
            SDL_RenderDrawLine(rend, tempPoints[i - 1].first, tempPoints[i - 1].second,
                tempPoints[i].first, tempPoints[i].second); // Fixed: was points[i], now tempPoints[i]
        }
    }
    else if (tempPoints.size() == 1) {
        SDL_RenderDrawPoint(rend, tempPoints[0].first, tempPoints[0].second);
    }
}

void drawEraserRealtime(SDL_Renderer* rend) {
    SDL_SetRenderDrawColor(rend, 255, 255, 255, 255); // White color for eraser

    for (const auto& era : eraserCurves) {
        for (int dx = -10; dx <= 10; ++dx) {
            for (int dy = -10; dy <= 10; ++dy) {
                if (dx * dx + dy * dy <= 100) { // Circle with radius 10
                    int px = era.first + dx;
                    int py = era.second + dy;
                    if (px >= 0 && px < CANVAS_WIDTH && py >= 0 && py < WINDOW_HEIGHT) {
                        SDL_RenderDrawPoint(rend, px, py);
                    }
                }
            }
        }
    }
}

void drawEraser(SDL_Renderer* rend, const std::vector<std::pair<int, int>>& tempEraserCurves) {
    SDL_SetRenderDrawColor(rend, 255, 255, 255, 255); // White color for eraser

    for (const auto& era : tempEraserCurves) {
        for (int dx = -10; dx <= 10; ++dx) {
            for (int dy = -10; dy <= 10; ++dy) {
                if (dx * dx + dy * dy <= 100) { // Circle with radius 10
                    int px = era.first + dx;
                    int py = era.second + dy;
                    if (px >= 0 && px < CANVAS_WIDTH && py >= 0 && py < WINDOW_HEIGHT) {
                        SDL_RenderDrawPoint(rend, px, py);
                    }
                }
            }
        }
    }
}

void drawLineTool(SDL_Renderer* rend, const std::vector<std::pair<int, int>>& tempLinePoints, const Uint8* tempColor) {
    SDL_SetRenderDrawColor(rend, tempColor[0], tempColor[1], tempColor[2], 255);

    if (tempLinePoints.size() >= 2) {
        SDL_RenderDrawLine(rend, tempLinePoints[0].first, tempLinePoints[0].second,
            tempLinePoints[1].first, tempLinePoints[1].second);
    }
}

void lineToolRealtime(SDL_Renderer* rend) {
    SDL_SetRenderDrawColor(rend, currColor[0], currColor[1], currColor[2], 255);

    if (lineCurves.size() >= 2) {
        SDL_RenderDrawLine(rend, lineCurves[0].first, lineCurves[0].second,
            lineCurves[1].first, lineCurves[1].second);
    }
}

void rectangleToolRealtime(SDL_Renderer* rend) {
    SDL_SetRenderDrawColor(rend, currColor[0], currColor[1], currColor[2], 255);

    if (rectPoints.size() >= 5) {
        for (size_t i = 1; i < rectPoints.size(); ++i) {
            SDL_RenderDrawLine(rend, rectPoints[i - 1].first, rectPoints[i - 1].second,
                rectPoints[i].first, rectPoints[i].second);
        }
    }
}

void rectToolP(SDL_Renderer* rend, const std::vector<std::pair<int, int>>& tempRectPoints, const Uint8* tempColor) {
    SDL_SetRenderDrawColor(rend, tempColor[0], tempColor[1], tempColor[2], 255);

    if (tempRectPoints.size() >= 5) {
        for (size_t i = 1; i < tempRectPoints.size(); ++i) {
            SDL_RenderDrawLine(rend, tempRectPoints[i - 1].first, tempRectPoints[i - 1].second,
                tempRectPoints[i].first, tempRectPoints[i].second);
        }
    }
}

void setBool(bool eraser, bool pencil, bool line, bool paint, bool rect) {
    pencilTool = pencil;
    eraserTool = eraser;
    lineTool = line;
    paintTool = paint;
    rectTool = rect;
}

void handleKeyboard(SDL_Keycode key) {
    switch (key) {
    case SDLK_e:
        setBool(true, false, false, false, false);
        break;
    case SDLK_p:
        setBool(false, true, false, false, false);
        break;
    case SDLK_q:
        setBool(false, false, false, false, true);
        break;
    case SDLK_l:
        setBool(false, false, true, false, false);
        break;
    case SDLK_f:
        setBool(false, false, false, true, false);
        break;
    case SDLK_g:
        currColor[0] = 0;   // Green
        currColor[1] = 255;
        currColor[2] = 0;
        break;
    case SDLK_r:
        currColor[0] = 255; // Red
        currColor[1] = 0;
        currColor[2] = 0;
        break;
    case SDLK_t:
        currColor[0] = 0;   // Black
        currColor[1] = 0;
        currColor[2] = 0;
        break;
    case SDLK_y:
        currColor[0] = 255; // Yellow
        currColor[1] = 255;
        currColor[2] = 0;
        break;
    case SDLK_b:
        //teal
		currColor[0] = 0;   // Teal
        currColor[1] = 255;
        currColor[2] = 255;
		break;
    case SDLK_i:
        //teal
        currColor[0] = 75;   // Teal
        currColor[1] = 0;
        currColor[2] = 130;
        break;
    case SDLK_z:
        if (!functionsToCall.empty()) {
            functionsToCall.pop_back();
        }
        break;
    case SDLK_ESCAPE: // Moved escape handling here for consistency
        SDL_Event quitEvent;
        quitEvent.type = SDL_QUIT;
        SDL_PushEvent(&quitEvent);
        break;
    }
}

void handleMouseClick(int button, int state, int x, int y) {
    if (button == SDL_BUTTON_LEFT && x < CANVAS_WIDTH && x >= 0 && y >= 0 && y < WINDOW_HEIGHT) { // Added bounds checking
        if (state == SDL_MOUSEBUTTONDOWN) {
            if (pencilTool) {
                points.clear(); // Clear previous points for safety
                points.push_back(std::make_pair(x, y));
                isDrawing = true;
            }
            if (eraserTool) {
                eraserCurves.clear(); // Clear previous points for safety
                eraserCurves.push_back(std::make_pair(x, y));
                isErasing = true;
            }
            if (lineTool) {
                lineCurves[0] = std::make_pair(x, y);
                lineCurves[1] = std::make_pair(x, y);
                isLining = true;
            }
            if (paintTool) {
                SDL_Surface* windowSurface = SDL_GetWindowSurface(window);
                if (windowSurface && windowSurface->pixels) { // Added null check
                    Uint32* pixels = (Uint32*)windowSurface->pixels;
                    // Bounds check for pixel access
                    if (x >= 0 && x < CANVAS_WIDTH && y >= 0 && y < WINDOW_HEIGHT) {
                        targetColor = pixels[y * windowSurface->pitch / 4 + x]; // Use proper pitch calculation

                        floodFillStack(x, y, currColor, targetColor);

                        if (!paintedPoints.empty()) { // Only add function if there are points to paint
                            std::vector<std::pair<int, int>> tempPaintingPoints = paintedPoints;
                            Uint8 tempPaintColor[3] = { currColor[0], currColor[1], currColor[2] };

                            functionsToCall.push_back([tempPaintingPoints, tempPaintColor](SDL_Renderer* rend) {
                                Uint8 colorPaintCopy[3] = { tempPaintColor[0], tempPaintColor[1], tempPaintColor[2] };
                                drawLine(rend, tempPaintingPoints, colorPaintCopy);
                                });
                        }
                        paintedPoints.clear();
                    }
                }
            }
            if (rectTool) {
                rectPoints.clear();
                rectPoints.resize(5); // Ensure proper size
                rectPoints[0] = std::make_pair(x, y);
                rectPoints[1] = std::make_pair(x, y);
                rectPoints[2] = std::make_pair(x, y);
                rectPoints[3] = std::make_pair(x, y);
                rectPoints[4] = std::make_pair(x, y);
                isDrawingRect = true;
            }
        }
        else if (state == SDL_MOUSEBUTTONUP) {
            if (pencilTool && isDrawing) {
                isDrawing = false;
                if (!points.empty()) { // Only add function if there are points to draw
                    std::vector<std::pair<int, int>> tempPoints = points;
                    Uint8 tempColor[3] = { currColor[0], currColor[1], currColor[2] };

                    functionsToCall.push_back([tempPoints, tempColor](SDL_Renderer* rend) {
                        Uint8 colorCopy[3] = { tempColor[0], tempColor[1], tempColor[2] };
                        drawLine(rend, tempPoints, colorCopy);
                        });
                }
                points.clear();
            }
            if (eraserTool && isErasing) {
                isErasing = false;
                if (!eraserCurves.empty()) { // Only add function if there are points to erase
                    std::vector<std::pair<int, int>> tempErasePoints = eraserCurves;
                    functionsToCall.push_back([tempErasePoints](SDL_Renderer* rend) {
                        drawEraser(rend, tempErasePoints);
                        });
                }
                eraserCurves.clear();
            }
            if (lineTool && isLining) {
                isLining = false;
                std::vector<std::pair<int, int>> tempLinePoints = lineCurves;
                Uint8 tempColor[3] = { currColor[0], currColor[1], currColor[2] };

                functionsToCall.push_back([tempLinePoints, tempColor](SDL_Renderer* rend) {
                    Uint8 colorCopy[3] = { tempColor[0], tempColor[1], tempColor[2] };
                    drawLineTool(rend, tempLinePoints, colorCopy);
                    });
                lineCurves = lineCurves1;
            }
            if (rectTool && isDrawingRect) {
                if (rectPoints.size() >= 5) {
                    rectPoints[1] = std::make_pair(rectPoints[0].first, y);
                    rectPoints[2] = std::make_pair(x, y);
                    rectPoints[3] = std::make_pair(x, rectPoints[0].second);
                    rectPoints[4] = std::make_pair(rectPoints[0].first, rectPoints[0].second);
                }
                isDrawingRect = false;

                Uint8 tempColor[3] = { currColor[0], currColor[1], currColor[2] };
                std::vector<std::pair<int, int>> tempRectanglePoints = rectPoints;

                functionsToCall.push_back([tempRectanglePoints, tempColor](SDL_Renderer* rend) {
                    Uint8 colorCopy[3] = { tempColor[0], tempColor[1], tempColor[2] };
                    rectToolP(rend, tempRectanglePoints, colorCopy);
                    });
                rectPoints.clear();
            }
        }
    }
}

void handleMouseMotion(int x, int y) {
    if (x < CANVAS_WIDTH && x >= 0 && y >= 0 && y < WINDOW_HEIGHT) { // Added bounds checking
        if (pencilTool && isDrawing) {
            points.push_back(std::make_pair(x, y));
        }
        if (eraserTool && isErasing) {
            eraserCurves.push_back(std::make_pair(x, y));
        }
        if (lineTool && isLining && lineCurves.size() >= 2) {
            lineCurves[1] = std::make_pair(x, y);
        }
        if (rectTool && isDrawingRect && rectPoints.size() >= 5) {
            rectPoints[1] = std::make_pair(rectPoints[0].first, y);
            rectPoints[2] = std::make_pair(x, y);
            rectPoints[3] = std::make_pair(x, rectPoints[0].second);
            rectPoints[4] = std::make_pair(rectPoints[0].first, rectPoints[0].second);
        }
    }
}

void drawTUI(SDL_Renderer* rend) {
    // Draw TUI background
    SDL_SetRenderDrawColor(rend, 220, 220, 220, 255); // Light gray
    SDL_Rect tuiRect = { CANVAS_WIDTH, 0, TUI_WIDTH, WINDOW_HEIGHT };
    SDL_RenderFillRect(rend, &tuiRect);

    // Draw dividing line
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
    SDL_RenderDrawLine(rend, CANVAS_WIDTH, 0, CANVAS_WIDTH, WINDOW_HEIGHT);

    if (!font) return; // Safety check for font

    // Prepare text
    struct MenuItem {
        std::string key;
        std::string desc;
        bool isActive;
        bool isColor;
        Uint8 color[3];
    };

    std::vector<MenuItem> menuItems = {
        {"P", "Pencil Tool", pencilTool, false, {0, 0, 0}},
        {"E", "Eraser Tool", eraserTool, false, {0, 0, 0}},
        {"L", "Line Tool", lineTool, false, {0, 0, 0}},
        {"F", "FloodFill Tool", paintTool, false, {0, 0, 0}},
        {"Q", "Rectangle Tool", rectTool, false, {0, 0, 0}},
        {"R", "Red Color", currColor[0] == 255 && currColor[1] == 0 && currColor[2] == 0, true, {255, 0, 0}},
        {"G", "Green Color", currColor[0] == 0 && currColor[1] == 255 && currColor[2] == 0, true, {0, 255, 0}},
        {"B", "Blue Color", currColor[0] == 0 && currColor[1] == 0 && currColor[2] == 255, true, {0, 255, 255}},
        {"Y", "Yellow Color", currColor[0] == 255 && currColor[1] == 255 && currColor[2] == 0, true, {255, 255, 0}},
        {"I", "Indigo Color", currColor[0] == 75 && currColor[1] == 0 && currColor[2] == 130, true, {75, 0, 130}},
        {"T", "Black Color", currColor[0] == 1 && currColor[1] == 1 && currColor[2] == 1, true, {1, 1, 1}},
        {"Z", "Undo", false, false, {0, 0, 0}},
        {"ESC", "Quit", false, false, {0, 0, 0}}
    };

    int yPos = 20;
    for (const auto& item : menuItems) {
        // Key text
        SDL_Color keyColor = item.isActive ? SDL_Color{ 255, 165, 0, 255 } : // Orange for active
            (item.isColor ? SDL_Color{ item.color[0], item.color[1], item.color[2], 255 } : SDL_Color{ 0, 0, 0, 255 });

        SDL_Surface* keySurface = TTF_RenderText_Solid(font, item.key.c_str(), keyColor);
        if (keySurface) { // Safety check
            SDL_Texture* keyTexture = SDL_CreateTextureFromSurface(rend, keySurface);
            if (keyTexture) { // Safety check
                int keyW, keyH;
                SDL_QueryTexture(keyTexture, nullptr, nullptr, &keyW, &keyH);
                SDL_Rect keyRect = { CANVAS_WIDTH + 10, yPos, keyW, keyH };
                SDL_RenderCopy(rend, keyTexture, nullptr, &keyRect); // SDL3 RenderTexture name makes much more sense now, all things considered.
                SDL_DestroyTexture(keyTexture);
            }
            SDL_FreeSurface(keySurface);
        }

        // Description text
        SDL_Surface* descSurface = TTF_RenderText_Solid(font, item.desc.c_str(), { 0, 0, 0, 255 });
        if (descSurface) { // Safety check
            SDL_Texture* descTexture = SDL_CreateTextureFromSurface(rend, descSurface);
            if (descTexture) { // Safety check
                int descW, descH;
                SDL_QueryTexture(descTexture, nullptr, nullptr, &descW, &descH);
                SDL_Rect descRect = { CANVAS_WIDTH + 50, yPos, descW, descH };
                SDL_RenderCopy(rend, descTexture, nullptr, &descRect);
                SDL_DestroyTexture(descTexture);
            }
            SDL_FreeSurface(descSurface);
        }

        yPos += 30;
    }
}

void render() {
    // Clear screen with white background
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // Draw all stored functions
    for (const auto& func : functionsToCall) {
        func(renderer);
    }

    // Draw real-time tools
    drawLineRealtime(renderer);
    lineToolRealtime(renderer);
    rectangleToolRealtime(renderer);
    drawEraserRealtime(renderer);

    // Draw TUI
    drawTUI(renderer);

    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Load font - try multiple common font paths
    const char* fontPaths[] = {
        "arial.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/System/Library/Fonts/Arial.ttf"
    };

    font = nullptr;
    for (const char* fontPath : fontPaths) {
        font = TTF_OpenFont(fontPath, 16);
        if (font) break;
    }

    if (font == nullptr) {
        std::cerr << "Failed to load any font! TTF_Error: " << TTF_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Create window
    window = SDL_CreateWindow("Paint",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN);

    if (window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Main loop
    bool quit = false;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    quit = true; // Exit on Escape key
                }
                else {
                    handleKeyboard(e.key.keysym.sym);
                }
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                handleMouseClick(e.button.button, SDL_MOUSEBUTTONDOWN, e.button.x, e.button.y);
            }
            else if (e.type == SDL_MOUSEBUTTONUP) {
                handleMouseClick(e.button.button, SDL_MOUSEBUTTONUP, e.button.x, e.button.y);
            }
            else if (e.type == SDL_MOUSEMOTION) {
                handleMouseMotion(e.motion.x, e.motion.y);
            }
        }

        render();
    }

    // Clean up
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
