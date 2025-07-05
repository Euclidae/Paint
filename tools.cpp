#include "tools.hpp"
#include "canvas.hpp"
#include "editor.hpp"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <queue>

/*
* I tried to make them work like photoshop tools.
*/
Tool Tools::currentTool = NONE;
bool Tools::isDrawing = false;
ImVec2 Tools::startPos, Tools::currentPos;
ImVec4 Tools::currColor(0.0f, 0.0f, 0.0f, 1.0f);
ImVec4 Tools::secondaryColor(1.0f, 1.0f, 1.0f, 1.0f);
int Tools::brushSize = 5;
int Tools::eraserSize = 20;
GradientType Tools::gradientType = LINEAR;

// Text tool variables - moved inside namespace below

// Text tool variables
std::vector<TextBox> Tools::textBoxes;
int Tools::activeTextBox = -1;

void Tools::init() {}
void Tools::cleanup() {}
// tools.cpp
namespace Tools {
    bool showHelpDialog = false;
    SDL_Rect selectionRect = {0, 0, 0, 0};
    bool hasSelection = false;

}

void Tools::handleSDLEvent(const SDL_Event& e) {
    using namespace Canvas;
    if (e.type == SDL_KEYDOWN) {
        SDL_Keymod mod = SDL_GetModState();
        switch (e.key.keysym.sym) {
            case SDLK_e: currentTool = ERASER; break;
            case SDLK_p: currentTool = PENCIL; break;
            case SDLK_r: currentTool = RECT; break;
            case SDLK_l: currentTool = LINE; break;
            case SDLK_f: currentTool = FILL; break;

            case SDLK_t: currentTool = TRIANGLE; break;
            case SDLK_g: currentTool = GRADIENT; break;
            case SDLK_s: currentTool = SELECT; break;
            case SDLK_x: std::swap(currColor, secondaryColor); break; // Swap primary/secondary colors

            case SDLK_b: currColor = ImVec4(0.0f, 0.0f, 1.0f, 1.0f); break; // Quick blue color
            case SDLK_ESCAPE: currentTool = NONE; break;
            case SDLK_PLUS:
            case SDLK_EQUALS:
                if (mod & KMOD_CTRL) brushSize = std::min(50, brushSize + 1); // Increase brush size
                else if (activeLayerIndex < static_cast<int>(layers.size()) - 1) activeLayerIndex++; // Next layer
                break;
            case SDLK_MINUS:
                if (mod & KMOD_CTRL) brushSize = std::max(1, brushSize - 1); // Decrease brush size
                else if (activeLayerIndex > 0) activeLayerIndex--; // Previous layer
                break;
            case SDLK_z:
                if (mod & KMOD_CTRL) {
                    if (mod & KMOD_SHIFT) Editor::applyRedo(); // Ctrl+Shift+Z: Redo
                    else Editor::applyUndo(); // Ctrl+Z: Undo
                }
                break;
            case SDLK_y:
                if (mod & KMOD_CTRL) Editor::applyRedo(); // Ctrl+Y: Redo
                break;
            case SDLK_m:
                if (mod & KMOD_CTRL) Editor::mergeLayers(); // Ctrl+M: Merge layers
                break;
            case SDLK_DELETE:
                Editor::deleteSelection(); // Delete selection
                break;
            case SDLK_c:
                if (mod & KMOD_CTRL) Editor::copySelection(); // Ctrl+C: Copy selection
                else currentTool = CIRCLE; // C: Circle tool
                break;
            case SDLK_v:
                if (mod & KMOD_CTRL) Editor::pasteSelection(); // Ctrl+V: Paste selection
                else currColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // V: Quick green color
                break;
            case SDLK_h:
                if (!(mod & KMOD_CTRL)) {
                    // Open help dialog (requires ImGui context)
                    // This will be handled by the UI system
                    extern bool showHelpDialog;
                    showHelpDialog = true;
                }
                break;
        }
    }
    if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
        int x = e.button.x - 10, y = e.button.y - 10; // Account for canvas offset
        if (x < 0 || y < 0 || x >= CANVAS_WIDTH || y >= CANVAS_HEIGHT) return; // Check canvas bounds
        if (e.button.button == SDL_BUTTON_LEFT) {
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                // Check if clicking on existing text box
                if (currentTool == TEXT) {
                    for (int i = 0; i < (int)textBoxes.size(); i++) {
                        TextBox& textBox = textBoxes[i];
                        if (x >= textBox.rect.x && x <= textBox.rect.x + textBox.rect.w &&
                            y >= textBox.rect.y && y <= textBox.rect.y + textBox.rect.h) {
                            // Activate this text box for editing
                            activeTextBox = i;
                            textBox.isActive = true;
                            return; // Don't create a new text box
                        }
                    }
                }

                isDrawing = true;
                startPos = ImVec2((float)x, (float)y);
                currentPos = startPos;
                if (currentTool == FILL) {
                    Editor::saveUndoState();
                    floodFill(x, y, currColor);
                }
                if (currentTool == GRADIENT || currentTool == PENCIL || currentTool == ERASER || currentTool == TEXT)
                    Editor::saveUndoState();
            } else if (e.type == SDL_MOUSEBUTTONUP) {
                isDrawing = false;
                if (currentTool != NONE && currentTool != FILL) {
                    SDL_SetRenderTarget(renderer, layers[activeLayerIndex].texture);
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    switch (currentTool) {
                        case LINE: drawLine(renderer, startPos, currentPos, currColor, brushSize); break;
                        case RECT: drawRectangle(renderer, startPos, currentPos, currColor, false); break;
                        case CIRCLE: {
                            int radius = static_cast<int>(sqrt(pow(currentPos.x - startPos.x, 2) + pow(currentPos.y - startPos.y, 2)));
                            drawCircle(renderer, startPos, radius, currColor, false);
                            break;
                        }
                        case TRIANGLE: {
                            ImVec2 p3 = ImVec2(startPos.x, currentPos.y);
                            drawTriangle(renderer, startPos, currentPos, p3, currColor);
                            break;
                        }
                        case GRADIENT: drawGradient(renderer, startPos, currentPos, currColor, secondaryColor, gradientType); break;
                        case TEXT: {
                            // Create text box
                            int x = (int)std::min(startPos.x, currentPos.x);
                            int y = (int)std::min(startPos.y, currentPos.y);
                            int w = std::max(100, (int)std::abs(currentPos.x - startPos.x));
                            int h = std::max(30, (int)std::abs(currentPos.y - startPos.y));
                            createTextBox(x, y, w, h);
                            break;
                        }
                        case SELECT: {
                            // Finalize selection rectangle
                            extern SDL_Rect selectionRect;
                            extern bool hasSelection;
                            selectionRect.w = (int)std::abs(currentPos.x - startPos.x);
                            selectionRect.h = (int)std::abs(currentPos.y - startPos.y);
                            selectionRect.x = (int)std::min(startPos.x, currentPos.x);
                            selectionRect.y = (int)std::min(startPos.y, currentPos.y);
                            hasSelection = true;
                            Editor::copySelection();
                            ::selectionOffset = ImVec2(0, 0);
                            break;
                        }
                        case PENCIL: case ERASER: break;
                        default: break;
                    }
                    SDL_SetRenderTarget(renderer, nullptr);
                }
            }
        }
        else if (e.button.button == SDL_BUTTON_RIGHT && e.type == SDL_MOUSEBUTTONDOWN) {
            // Right click: Color picker (eyedropper tool)
            SDL_SetRenderTarget(renderer, canvasBuffer);
            SDL_Rect pixel = {x, y, 1, 1};
            Uint32 pixelColor;
            if (SDL_RenderReadPixels(renderer, &pixel, SDL_PIXELFORMAT_RGBA8888, &pixelColor, sizeof(Uint32)) == 0) {
                Uint8 r, g, b, a;
                SDL_GetRGBA(pixelColor, SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888), &r, &g, &b, &a);
                secondaryColor = ImVec4(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
            }
            SDL_SetRenderTarget(renderer, nullptr);
        }
    }
    if (e.type == SDL_MOUSEMOTION) {
        int x = e.motion.x - 10, y = e.motion.y - 10; // Account for canvas offset
        if (x < 0 || y < 0 || x >= CANVAS_WIDTH || y >= CANVAS_HEIGHT) return;
        currentPos = ImVec2((float)x, (float)y);
        if (isDrawing) {
            if (currentTool == PENCIL || currentTool == ERASER) {
                SDL_SetRenderTarget(renderer, layers[activeLayerIndex].texture);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                if (currentTool == PENCIL) {
                    // Draw continuous line for smooth drawing
                    SDL_SetRenderDrawColor(renderer, (Uint8)(currColor.x * 255), (Uint8)(currColor.y * 255), (Uint8)(currColor.z * 255), 255);
                    SDL_RenderDrawLine(renderer, (int)startPos.x, (int)startPos.y, (int)currentPos.x, (int)currentPos.y);
                } else if (currentTool == ERASER) {
                    drawEraser(renderer, currentPos, eraserSize);
                }
                SDL_SetRenderTarget(renderer, nullptr);
                startPos = currentPos; // Update start position for continuous drawing
            }
        }
    }
    if (e.type == SDL_MOUSEWHEEL) {
        // Mouse wheel: Adjust brush size
        if (e.wheel.y > 0) brushSize = std::min(50, brushSize + 1);
        else if (e.wheel.y < 0) brushSize = std::max(1, brushSize - 1);
    }
}


void Tools::drawLine(SDL_Renderer* rend, ImVec2 start, ImVec2 end, const ImVec4& color, int thickness) {
    SDL_SetRenderDrawColor(rend, (Uint8)(color.x * 255), (Uint8)(color.y * 255), (Uint8)(color.z * 255), (Uint8)(color.w * 255));
    if (thickness == 1) {
        // Simple line for thickness 1
        SDL_RenderDrawLine(rend, (int)start.x, (int)start.y, (int)end.x, (int)end.y);
    } else {
        // Thick line implementation using multiple parallel lines
        float dx = end.x - start.x, dy = end.y - start.y;
        float length = sqrt(dx*dx + dy*dy);
        if (length < 0.001) return; // Avoid division by zero
        dx /= length; dy /= length;
        float perpX = -dy * (thickness / 2.0f), perpY = dx * (thickness / 2.0f);
        for (int i = 0; i < thickness; i++) {
            float offsetX = perpX * (i - thickness/2.0f);
            float offsetY = perpY * (i - thickness/2.0f);
            SDL_RenderDrawLine(rend, (int)(start.x + offsetX), (int)(start.y + offsetY), (int)(end.x + offsetX), (int)(end.y + offsetY));
        }
    }
}

void Tools::drawEraser(SDL_Renderer* rend, ImVec2 position, int size) {
    using namespace Canvas;
    SDL_SetRenderTarget(rend, layers[activeLayerIndex].texture);
    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_NONE); // Use NONE to actually erase (make transparent)
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 0); // Transparent color
    int centerX = (int)position.x, centerY = (int)position.y, radius = size / 2, radiusSquared = radius * radius;
    // Draw circular eraser shape
    for (int y = -radius; y <= radius; y++)
        for (int x = -radius; x <= radius; x++)
            if (x*x + y*y <= radiusSquared) // Check if point is within circle
                SDL_RenderDrawPoint(rend, centerX + x, centerY + y);
    SDL_SetRenderTarget(rend, nullptr);
}

void Tools::drawRectangle(SDL_Renderer* rend, ImVec2 start, ImVec2 end, const ImVec4& color, bool filled) {
    SDL_SetRenderDrawColor(rend, (Uint8)(color.x * 255), (Uint8)(color.y * 255), (Uint8)(color.z * 255), (Uint8)(color.w * 255));
    SDL_Rect rect = {(int)std::min(start.x, end.x), (int)std::min(start.y, end.y), (int)std::abs(end.x - start.x), (int)std::abs(end.y - start.y)};
    if (filled) SDL_RenderFillRect(rend, &rect);
    else SDL_RenderDrawRect(rend, &rect);
}

void Tools::drawCircle(SDL_Renderer* rend, ImVec2 center, int radius, const ImVec4& color, bool filled) {
    SDL_SetRenderDrawColor(rend, (Uint8)(color.x * 255), (Uint8)(color.y * 255), (Uint8)(color.z * 255), (Uint8)(color.w * 255));
    if (filled) {
        for (int w = -radius; w < radius; w++)
            for (int h = -radius; h < radius; h++)
                if (w*w + h*h <= radius*radius)
                    SDL_RenderDrawPoint(rend, (int)center.x + w, (int)center.y + h);
    } else {
        int x = radius, y = 0, err = 0;
        while (x >= y) {
            SDL_RenderDrawPoint(rend, (int)center.x + x, (int)center.y + y);
            SDL_RenderDrawPoint(rend, (int)center.x + y, (int)center.y + x);
            SDL_RenderDrawPoint(rend, (int)center.x - y, (int)center.y + x);
            SDL_RenderDrawPoint(rend, (int)center.x - x, (int)center.y + y);
            SDL_RenderDrawPoint(rend, (int)center.x - x, (int)center.y - y);
            SDL_RenderDrawPoint(rend, (int)center.x - y, (int)center.y - x);
            SDL_RenderDrawPoint(rend, (int)center.x + y, (int)center.y - x);
            SDL_RenderDrawPoint(rend, (int)center.x + x, (int)center.y - y);
            if (err <= 0) { y += 1; err += 2*y + 1; }
            if (err > 0) { x -= 1; err -= 2*x + 1; }
        }
    }
}

void Tools::drawTriangle(SDL_Renderer* rend, ImVec2 p1, ImVec2 p2, ImVec2 p3, const ImVec4& color) {
    SDL_SetRenderDrawColor(rend, (Uint8)(color.x * 255), (Uint8)(color.y * 255), (Uint8)(color.z * 255), (Uint8)(color.w * 255));
    SDL_RenderDrawLine(rend, (int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y);
    SDL_RenderDrawLine(rend, (int)p2.x, (int)p2.y, (int)p3.x, (int)p3.y);
    SDL_RenderDrawLine(rend, (int)p3.x, (int)p3.y, (int)p1.x, (int)p1.y);
}

void Tools::createTextBox(int x, int y, int w, int h) {
    // Create a new layer for this text box
    std::string layerName = "Text " + std::to_string(textBoxes.size() + 1);
    Canvas::addLayer(layerName);

    TextBox newTextBox;
    newTextBox.rect = {x, y, w, h};
    newTextBox.content = "Enough is cute.";
    newTextBox.fontSize = 24;
    newTextBox.bold = false;
    newTextBox.italic = false;
    newTextBox.isActive = true;
    newTextBox.color = currColor;
    newTextBox.layerIndex = Canvas::activeLayerIndex;  // Associate with the new layer

    textBoxes.push_back(newTextBox);
    activeTextBox = (int)textBoxes.size() - 1;
}

void Tools::renderTextBoxes(SDL_Renderer* rend) {
    using namespace Canvas;
    for (const auto& textBox : textBoxes) {
        // Render text to the layer it belongs to
        if (textBox.layerIndex >= 0 && textBox.layerIndex < (int)layers.size()) {
            SDL_SetRenderTarget(rend, layers[textBox.layerIndex].texture);
            renderTextBox(rend, textBox);
            SDL_SetRenderTarget(rend, nullptr);
        }
    }
}

void Tools::renderTextBox(SDL_Renderer* rend, const TextBox& textBox) {
    if (textBox.content.empty()) return;

    TTF_Font* font = nullptr;

    // Try to load custom font first, then fall back to system fonts.
    // TODO - add download functionality to the python script.
    // Update - Done.
    if (!textBox.fontPath.empty()) {
        font = TTF_OpenFont(textBox.fontPath.c_str(), textBox.fontSize);
        if (font) {
            int style = TTF_STYLE_NORMAL;
            if (textBox.bold) style |= TTF_STYLE_BOLD;
            if (textBox.italic) style |= TTF_STYLE_ITALIC;
            TTF_SetFontStyle(font, style);
        }
    }

    // Fall back to system fonts if custom font failed or not specified
    if (!font) {
        font = Canvas::getFont(textBox.fontSize, textBox.bold, textBox.italic);
    }

    if (!font) return;

    SDL_Color color = {
        (Uint8)(textBox.color.x * 255),
        (Uint8)(textBox.color.y * 255),
        (Uint8)(textBox.color.z * 255),
        (Uint8)(textBox.color.w * 255)
    };

    SDL_Surface* textSurface = TTF_RenderText_Blended(font, textBox.content.c_str(), color);
    if (!textSurface) return;

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(rend, textSurface);
    if (!textTexture) {
        SDL_FreeSurface(textSurface);
        return;
    }

    SDL_Rect destRect = {textBox.rect.x, textBox.rect.y, textSurface->w, textSurface->h};
    SDL_RenderCopy(rend, textTexture, nullptr, &destRect);

    // Draw border if active
    if (textBox.isActive) {
        SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
        SDL_RenderDrawRect(rend, &textBox.rect);
    }

    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);

    // Clean up custom font (system fonts are cached and managed by Canvas)
    if (!textBox.fontPath.empty() && font) {
        TTF_CloseFont(font);
    }
}



void Tools::drawGradient(SDL_Renderer* rend, ImVec2 start, ImVec2 end, const ImVec4& color1, const ImVec4& color2, GradientType type) {
    // https://graphicdesign.stackexchange.com/questions/54880/what-is-the-math-behind-gradient-map-in-photoshop. Shout out to the maths guys from here
    int steps = 100;
    float dx = end.x - start.x, dy = end.y - start.y;
    float distance = sqrt(dx*dx + dy*dy);
    if (distance < 1.0f) return;
    dx /= distance; dy /= distance;
    for (int i = 0; i < steps; i++) {
        float t = i / (float)(steps - 1);
        ImVec4 color;
        color.x = color1.x + t * (color2.x - color1.x);
        color.y = color1.y + t * (color2.y - color1.y);
        color.z = color1.z + t * (color2.z - color1.z);
        color.w = color1.w + t * (color2.w - color1.w);
        SDL_SetRenderDrawColor(rend, (Uint8)(color.x * 255), (Uint8)(color.y * 255), (Uint8)(color.z * 255), (Uint8)(color.w * 255));
        if (type == LINEAR) {

            ImVec2 pos = {start.x + t * dx * distance, start.y + t * dy * distance};
            SDL_RenderDrawPoint(rend, (int)pos.x, (int)pos.y);
        } else if (type == RADIAL) {
            float radius = t * distance;
            int centerX = (int)start.x, centerY = (int)start.y;
            for (int a = 0; a < 360; a += 5) {
                float rad = a * M_PI / 180.0f;
                int px = centerX + (int)(radius * cos(rad));
                int py = centerY + (int)(radius * sin(rad));
                SDL_RenderDrawPoint(rend, px, py);
            }
        } else if (type == ANGULAR) {
            float angle = t * 2 * M_PI;
            int px = start.x + (int)(distance * cos(angle));
            int py = start.y + (int)(distance * sin(angle));
            SDL_RenderDrawLine(rend, (int)start.x, (int)start.y, px, py);
        }
    }
}

void Tools::floodFill(int x, int y, ImVec4 fillColor) {
    using namespace Canvas;
    if (x < 0 || x >= CANVAS_WIDTH || y < 0 || y >= CANVAS_HEIGHT) return;
    if (layers.empty() || activeLayerIndex < 0 || activeLayerIndex >= static_cast<int>(layers.size())) return;
    if (!layers[activeLayerIndex].texture) return;

    // Get the target color at the clicked pixel
    Uint32 targetColor;
    SDL_SetRenderTarget(renderer, layers[activeLayerIndex].texture);
    SDL_Rect pixel = {x, y, 1, 1};
    if (SDL_RenderReadPixels(renderer, &pixel, SDL_PIXELFORMAT_RGBA8888, &targetColor, sizeof(Uint32)) != 0) {
        SDL_SetRenderTarget(renderer, nullptr);
        return;
    }

    // Convert fill color to SDL format
    SDL_PixelFormat* format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
    if (!format) {
        SDL_SetRenderTarget(renderer, nullptr);
        return;
    }
    Uint32 fillRGBA = SDL_MapRGBA(format, (Uint8)(fillColor.x * 255), (Uint8)(fillColor.y * 255), (Uint8)(fillColor.z * 255), 255);
    SDL_FreeFormat(format);

    // Don't fill if target and fill colors are the same
    if (targetColor == fillRGBA) {
        SDL_SetRenderTarget(renderer, nullptr);
        return;
    }

    // Flood fill algorithm using queue-based approach
    std::queue<std::pair<int, int>> pixels;
    pixels.push(std::make_pair(x, y));
    std::vector<std::vector<bool>> visited(CANVAS_WIDTH, std::vector<bool>(CANVAS_HEIGHT, false));
    SDL_SetRenderDrawColor(renderer, (Uint8)(fillColor.x * 255), (Uint8)(fillColor.y * 255), (Uint8)(fillColor.z * 255), 255);

    // Prevent infinite loops with iteration limit
    int maxIterations = CANVAS_WIDTH * CANVAS_HEIGHT * 300;
    int iterations = 0;

    while (!pixels.empty() && iterations < maxIterations) {
        iterations++;
        std::pair<int, int> p = pixels.front();
        pixels.pop();
        int px = p.first, py = p.second;

        // Skip if out of bounds or already visited
        if (px < 0 || px >= CANVAS_WIDTH || py < 0 || py >= CANVAS_HEIGHT || visited[px][py])
            continue;

        // Check if current pixel matches target color
        Uint32 currentColor;
        pixel = {px, py, 1, 1};
        if (SDL_RenderReadPixels(renderer, &pixel, SDL_PIXELFORMAT_RGBA8888, &currentColor, sizeof(Uint32)) != 0)
            break;
        if (currentColor != targetColor)
            continue;

        // Fill the pixel and mark as visited
        SDL_RenderDrawPoint(renderer, px, py);
        visited[px][py] = true;

        // Add adjacent pixels to queue
        if (px + 1 < CANVAS_WIDTH) pixels.push(std::make_pair(px + 1, py));
        if (px - 1 >= 0) pixels.push(std::make_pair(px - 1, py));
        if (py + 1 < CANVAS_HEIGHT) pixels.push(std::make_pair(px, py + 1));
        if (py - 1 >= 0) pixels.push(std::make_pair(px, py - 1));
    }
    SDL_SetRenderTarget(renderer, nullptr);
}

// Some getter methods with inconsistent naming
const std::vector<TextBox>& Tools::get_text_boxes() {
    return textBoxes;
}

int Tools::getActiveTextBoxIndex() {
    return activeTextBox;
}

size_t Tools::textBoxCount() {
    return textBoxes.size();
}
