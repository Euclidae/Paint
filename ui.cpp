#include "ui.hpp"
#include "canvas.hpp"
#include "tools.hpp"
#include "editor.hpp"
#include "imgui/imgui.h"
#include "tinyfiledialogs/tinyfiledialogs.h"
#include <cstring>
#include <algorithm>

bool showHelpDialog = false;

void UI::init() {}
void UI::cleanup() {}

void UI::render() {
    using namespace Canvas;
    using namespace Tools;
    using Canvas::Layer;

    // File menu
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Canvas")) {
                setupNewCanvas(1280, 720);
            }
            if (ImGui::MenuItem("Open Image")) {
                const char* filePath = tinyfd_openFileDialog("Open Image File", "", 0, nullptr, "Image files", 0);
                if (filePath) importImage(filePath);
            }
            if (ImGui::BeginMenu("Save As")) {
                if (ImGui::MenuItem("PNG")) {
                    const char* filePath = tinyfd_saveFileDialog("Save Image as PNG", "untitled.png", 0, nullptr, "PNG files");
                    if (filePath) exportImage(filePath, "png");
                }
                if (ImGui::MenuItem("JPEG")) {
                    const char* filePath = tinyfd_saveFileDialog("Save Image as JPEG", "untitled.jpg", 0, nullptr, "JPEG files");
                    if (filePath) exportImage(filePath, "jpg");
                }
                if (ImGui::MenuItem("BMP")) {
                    const char* filePath = tinyfd_saveFileDialog("Save Image as BMP", "untitled.bmp", 0, nullptr, "BMP files");
                    if (filePath) exportImage(filePath, "bmp");
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Exit")) {
                SDL_Event quitEvent;
                quitEvent.type = SDL_QUIT;
                SDL_PushEvent(&quitEvent);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) Editor::applyUndo();
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) Editor::applyRedo();
            ImGui::Separator();
            if (ImGui::MenuItem("Copy", "Ctrl+C", false, ::hasSelection)) Editor::copySelection();
            if (ImGui::MenuItem("Paste", "Ctrl+V")) Editor::pasteSelection();
            if (ImGui::MenuItem("Clear Selection", "Delete", false, ::hasSelection)) Editor::deleteSelection();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) ImGui::OpenPopup("About");
            if (ImGui::MenuItem("Keyboard Shortcuts", "H")) ImGui::OpenPopup("Shortcuts");
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Advanced Image Editor");
        ImGui::Text("Version 1.0");
        ImGui::Text("Created with SDL2, Dear ImGui, and TinyFileDialogs");
        ImGui::Separator();
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // Handle H key shortcut to show help
    if (showHelpDialog) {
        ImGui::OpenPopup("Shortcuts");
        showHelpDialog = false;
    }

    // Keyboard shortcuts help dialog
    if (ImGui::BeginPopupModal("Shortcuts", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Keyboard Shortcuts");
        ImGui::Separator();
        ImGui::Text("Tools:");
        ImGui::BulletText("P - Pencil");
        ImGui::BulletText("E - Eraser");
        ImGui::BulletText("R - Rectangle");
        ImGui::BulletText("L - Line");
        ImGui::BulletText("F - Fill");
        ImGui::BulletText("C - Circle");
        ImGui::BulletText("T - Triangle");
        ImGui::BulletText("G - Gradient");
        ImGui::BulletText("S - Select");
        ImGui::BulletText("ESC - Clear selection");
        ImGui::Separator();
        ImGui::Text("Colors:");
        ImGui::BulletText("X - Swap primary/secondary colors");
        ImGui::BulletText("V - Quick green color");
        ImGui::BulletText("B - Quick blue color");
        ImGui::BulletText("Right click - Color picker");
        ImGui::Separator();
        ImGui::Text("Editing:");
        ImGui::BulletText("Ctrl+Z - Undo");
        ImGui::BulletText("Ctrl+Y / Ctrl+Shift+Z - Redo");
        ImGui::BulletText("Ctrl+M - Merge layers");
        ImGui::BulletText("Ctrl+C - Copy selection");
        ImGui::BulletText("Ctrl+V - Paste selection");
        ImGui::BulletText("Delete - Delete selection");
        ImGui::Separator();
        ImGui::Text("Navigation:");
        ImGui::BulletText("Ctrl+Plus/Minus - Adjust brush size");
        ImGui::BulletText("Plus/Minus - Navigate layers");
        ImGui::BulletText("Mouse wheel - Adjust brush size");
        ImGui::Separator();
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(ImVec2(CANVAS_WIDTH + 20, 20));
    ImGui::SetNextWindowSize(ImVec2(TUI_WIDTH - 30, CANVAS_HEIGHT - 40));
    ImGui::Begin("Tools & Properties", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // Show current tool status
    ImGui::Text("Current Tool: %s",
        currentTool == PENCIL ? "Pencil" :
        currentTool == ERASER ? "Eraser" :
        currentTool == LINE ? "Line" :
        currentTool == RECT ? "Rectangle" :
        currentTool == CIRCLE ? "Circle" :
        currentTool == TRIANGLE ? "Triangle" :
        currentTool == FILL ? "Fill" :
        currentTool == TEXT ? "Text" :
        currentTool == SELECT ? "Select" :
        currentTool == GRADIENT ? "Gradient" : "None");

    // Show selection status
    if (::hasSelection) {
        ImGui::Text("Selection: %dx%d at (%d,%d)", 
            ::selectionRect.w, ::selectionRect.h, 
            ::selectionRect.x, ::selectionRect.y);
    } else {
        ImGui::Text("No selection");
    }
    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    if (ImGui::Button("Pencil (P)", ImVec2(120, 30))) currentTool = PENCIL;
    ImGui::SameLine();
    if (ImGui::Button("Eraser (E)", ImVec2(120, 30))) currentTool = ERASER;
    
    if (ImGui::Button("Line (L)", ImVec2(120, 30))) currentTool = LINE;
    ImGui::SameLine();
    if (ImGui::Button("Rectangle (R)", ImVec2(120, 30))) currentTool = RECT;
    
    if (ImGui::Button("Circle (C)", ImVec2(120, 30))) currentTool = CIRCLE;
    ImGui::SameLine();
    if (ImGui::Button("Triangle (T)", ImVec2(120, 30))) currentTool = TRIANGLE;
    
    if (ImGui::Button("Fill (F)", ImVec2(120, 30))) currentTool = FILL;
    ImGui::SameLine();
    if (currentTool == TEXT) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button("Text", ImVec2(120, 30))) currentTool = TEXT;
        ImGui::PopStyleColor();
    } else {
        if (ImGui::Button("Text", ImVec2(120, 30))) currentTool = TEXT;
    }
    
    if (currentTool == SELECT) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button("Select (S)", ImVec2(120, 30))) currentTool = SELECT;
        ImGui::PopStyleColor();
    } else {
        if (ImGui::Button("Select (S)", ImVec2(120, 30))) currentTool = SELECT;
    }
    ImGui::SameLine();
    if (ImGui::Button("Gradient (G)", ImVec2(120, 30))) currentTool = GRADIENT;
    
    if (ImGui::Button("None (ESC)", ImVec2(120, 30))) currentTool = NONE;
    ImGui::PopStyleVar();

    ImGui::Separator();
    ImGui::Text("Tool Properties");
    if (currentTool == PENCIL || currentTool == ERASER || currentTool == LINE) {
        if (currentTool == ERASER) ImGui::SliderInt("Eraser Size", &eraserSize, 1, 100);
        else ImGui::SliderInt("Brush Size", &brushSize, 1, 50);
    }
    if (currentTool == GRADIENT) {
        const char* gradientTypes[] = { "Linear", "Radial", "Angular" };
        ImGui::Combo("Gradient Type", (int*)&gradientType, gradientTypes, 3);
    }
    if (currentTool == TEXT) {
        ImGui::Text("🔤 TEXT TOOL ACTIVE");
        ImGui::Separator();
        ImGui::Text("Click and drag to create text box");
        if (Tools::activeTextBox >= 0 && Tools::activeTextBox < (int)Tools::textBoxes.size()) {
            ImGui::Text("Active text box: %d", Tools::activeTextBox + 1);
        } else {
            ImGui::Text("No active text box");
        }
    }
    if (currentTool == SELECT) {
        ImGui::Text("📋 SELECT TOOL ACTIVE");
        ImGui::Separator();
        ImGui::Text("How to use:");
        ImGui::BulletText("Drag to select area");
        ImGui::BulletText("Use Ctrl+C to copy");
        ImGui::BulletText("Use Ctrl+V to paste");
        ImGui::BulletText("Press Delete to clear selection");
        ImGui::Separator();
        ImGui::Text("Selection Controls:");
        ImGui::Text("  Copy: Ctrl+C");
        ImGui::Text("  Paste: Ctrl+V");
        ImGui::Text("  Clear: Delete key");
    }
    ImGui::Separator();
    ImGui::Text("Colors (X to swap, V=green, B=blue)");
    ImGui::ColorEdit3("Primary Color", (float*)&currColor);
    if (currentTool == GRADIENT) ImGui::ColorEdit3("Secondary Color", (float*)&secondaryColor);
    ImGui::Text("Tip: Right-click on canvas to pick colors");

    ImGui::Separator();
    ImGui::Text("Layers");
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    if (ImGui::Button("+ Add", ImVec2(60, 25))) addLayer();
    ImGui::SameLine();
    if (ImGui::Button("Duplicate", ImVec2(80, 25)) && !layers.empty()) duplicateLayer(activeLayerIndex);
    ImGui::SameLine();
    if (ImGui::Button("- Remove", ImVec2(80, 25)) && layers.size() > 1) removeLayer(activeLayerIndex);
    ImGui::PopStyleVar();

    ImGui::BeginChild("LayerList", ImVec2(0, 200), true);
    for (size_t i = 0; i < layers.size(); i++) {
        ImGui::PushID(static_cast<int>(i));
        bool isActiveLayer = (static_cast<int>(i) == activeLayerIndex);
        if (isActiveLayer) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button(layers[i].name.c_str(), ImVec2(100, 0))) activeLayerIndex = static_cast<int>(i);
        if (isActiveLayer) ImGui::PopStyleColor();
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("LAYER_DRAG", &i, sizeof(size_t));
            ImGui::Text("Moving %s", layers[i].name.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LAYER_DRAG")) {
                size_t sourceIndex = *(const size_t*)payload->Data;
                moveLayer(static_cast<int>(sourceIndex), static_cast<int>(i));
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::SameLine();
        ImGui::PushID("visible");
        ImGui::Checkbox("##vis", &layers[i].visible);
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID("lock");
        ImGui::Checkbox("##lock", &layers[i].locked);
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::PushID("opacity");
        ImGui::SliderFloat("##opacity", &layers[i].opacity, 0.0f, 1.0f, "%.2f");
        ImGui::PopID();
        ImGui::PopID();
    }
    ImGui::EndChild();

    if (!layers.empty()) {
        ImGui::Separator();
        ImGui::Text("Blend Mode");
        const char* blendModes[] = {"Normal", "Multiply", "Add", "Screen"};
        ImGui::Combo("##BlendMode", &layers[activeLayerIndex].blendMode, blendModes, 4);
    }
    ImGui::Separator();
    ImGui::Text("Canvas Size: %d x %d", CANVAS_WIDTH, CANVAS_HEIGHT);
    static int newWidth = CANVAS_WIDTH;
    static int newHeight = CANVAS_HEIGHT;
    ImGui::InputInt("Width", &newWidth);
    ImGui::InputInt("Height", &newHeight);
    newWidth = std::max(100, newWidth);
    newHeight = std::max(100, newHeight);
    if (ImGui::Button("Resize Canvas")) setupNewCanvas(newWidth, newHeight);

    // Status bar with helpful information
    ImGui::Separator();
    ImGui::Text("Status Bar");
    ImGui::Separator();

    // Show current tool tips
    if (currentTool == PENCIL) {
        ImGui::Text("💡 Hold mouse and drag to draw. Use mouse wheel to adjust brush size.");
    } else if (currentTool == ERASER) {
        ImGui::Text("💡 Hold mouse and drag to erase. Adjust eraser size in properties.");
    } else if (currentTool == TEXT) {
        ImGui::Text("💡 Drag to create text box, then edit in popup window.");
    } else if (currentTool == SELECT) {
        ImGui::Text("💡 Drag to select area. Use Ctrl+C/V to copy/paste.");
    } else if (currentTool == FILL) {
        ImGui::Text("💡 Click to fill area with current color.");
    } else if (currentTool == GRADIENT) {
        ImGui::Text("💡 Drag to create gradient between primary/secondary colors.");
    } else if (currentTool == RECT || currentTool == CIRCLE || currentTool == TRIANGLE || currentTool == LINE) {
        ImGui::Text("💡 Drag to draw shape. Hold Shift for perfect proportions.");
    } else {
        ImGui::Text("💡 Select a tool to start drawing. Press H for help.");
    }

    // Show current key hint
    ImGui::Text("Quick Keys: P=Pencil, E=Eraser, T=Text, S=Select, H=Help");

    // Show memory usage info
    ImGui::Text("Layers: %d | Active: %d", (int)layers.size(), activeLayerIndex + 1);

    ImGui::End();

    // Text Box Editor Window
    if (Tools::activeTextBox >= 0 && Tools::activeTextBox < (int)Tools::textBoxes.size()) {
        ImGui::SetNextWindowSize(ImVec2(350, 250), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(CANVAS_WIDTH + 50, 300), ImGuiCond_FirstUseEver);
        
        bool showEditor = true;
        if (ImGui::Begin("Text Editor", &showEditor)) {
            TextBox& textBox = Tools::textBoxes[Tools::activeTextBox];
            
            // Text input
            static char textBuffer[512];
            strncpy(textBuffer, textBox.content.c_str(), sizeof(textBuffer) - 1);
            textBuffer[sizeof(textBuffer) - 1] = '\0';
            
            if (ImGui::InputTextMultiline("Text", textBuffer, sizeof(textBuffer), ImVec2(-1, 100))) {
                textBox.content = std::string(textBuffer);
            }
            
            ImGui::Separator();
            
            // Font settings
            ImGui::SliderInt("Font Size", &textBox.fontSize, 8, 72);
            ImGui::Checkbox("Bold", &textBox.bold);
            ImGui::SameLine();
            ImGui::Checkbox("Italic", &textBox.italic);
            
            // Color picker
            ImGui::ColorEdit3("Color", (float*)&textBox.color);
            
            ImGui::Separator();
            
            // Font selection
            ImGui::Text("Font Settings");
            ImGui::Text("Current: %s", textBox.fontPath.empty() ? "Default System Font" : textBox.fontPath.c_str());
            
            // Font presets - cross-platform paths
            ImGui::Text("Quick Presets:");
            if (ImGui::Button("Sans Serif")) {
                // Try common paths for sans-serif fonts
                const char* sansPaths[] = {
                    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",  // Linux
                    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",                  // Linux
                    "/System/Library/Fonts/Arial.ttf",                                 // macOS
                    "C:/Windows/Fonts/arial.ttf",                                      // Windows
                    nullptr
                };
                for (int i = 0; sansPaths[i]; i++) {
                    TTF_Font* testFont = TTF_OpenFont(sansPaths[i], 16);
                    if (testFont) {
                        TTF_CloseFont(testFont);
                        textBox.fontPath = sansPaths[i];
                        break;
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Serif")) {
                const char* serifPaths[] = {
                    "/usr/share/fonts/truetype/liberation/LiberationSerif-Regular.ttf", // Linux
                    "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf",                 // Linux
                    "/System/Library/Fonts/Times.ttf",                                 // macOS
                    "C:/Windows/Fonts/times.ttf",                                      // Windows
                    nullptr
                };
                for (int i = 0; serifPaths[i]; i++) {
                    TTF_Font* testFont = TTF_OpenFont(serifPaths[i], 16);
                    if (testFont) {
                        TTF_CloseFont(testFont);
                        textBox.fontPath = serifPaths[i];
                        break;
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Monospace")) {
                const char* monoPaths[] = {
                    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",  // Linux
                    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",              // Linux
                    "/System/Library/Fonts/Monaco.ttf",                                // macOS
                    "C:/Windows/Fonts/consola.ttf",                                    // Windows
                    nullptr
                };
                for (int i = 0; monoPaths[i]; i++) {
                    TTF_Font* testFont = TTF_OpenFont(monoPaths[i], 16);
                    if (testFont) {
                        TTF_CloseFont(testFont);
                        textBox.fontPath = monoPaths[i];
                        break;
                    }
                }
            }
            
            if (ImGui::Button("Choose Font File")) {
                const char* filter[] = { "*.ttf", "*.otf" };
                const char* path = tinyfd_openFileDialog("Choose Font", "", 2, filter, "Font Files", 0);
                if (path) {
                    textBox.fontPath = std::string(path);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset to Default")) {
                textBox.fontPath = "";
            }
            
            ImGui::Separator();
            
            // Position and size
            ImGui::Text("Position & Size");
            ImGui::SliderInt("X", &textBox.rect.x, 0, CANVAS_WIDTH - textBox.rect.w);
            ImGui::SliderInt("Y", &textBox.rect.y, 0, CANVAS_HEIGHT - textBox.rect.h);
            ImGui::SliderInt("Width", &textBox.rect.w, 50, CANVAS_WIDTH);
            ImGui::SliderInt("Height", &textBox.rect.h, 20, CANVAS_HEIGHT);
            
            ImGui::Separator();
            
            if (ImGui::Button("Done")) {
                textBox.isActive = false;
                Tools::activeTextBox = -1;
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                Tools::textBoxes.erase(Tools::textBoxes.begin() + Tools::activeTextBox);
                Tools::activeTextBox = -1;
            }
        }
        ImGui::End();
        
        if (!showEditor) {
            Tools::textBoxes[Tools::activeTextBox].isActive = false;
            Tools::activeTextBox = -1;
        }
    }
}
