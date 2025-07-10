#include "ui.hpp"
#include "canvas.hpp"
#include "tools.hpp"
#include "editor.hpp"
#include "imgui/imgui.h"
#include "tinyfiledialogs/tinyfiledialogs.h"
#include <cstring>
#include <algorithm>

extern int resizeCorner;

bool showHelpDialog = false;
bool showResizeDialog = false;
bool showContrastDialog = false;
bool showAboutDialog = false;
bool showHueSaturationDialog = false;
bool showBlurDialog = false;

void UI::init() {
    // stole this from this guy. Looks neat - https://github.com/GraphicsProgramming/dear-imgui-styles

    ImGui::CreateContext();

    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.FrameRounding = 3.0f;
    style.Colors[ImGuiCol_Text]                  = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // Updated from ChildWindowBg
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.86f, 0.86f, 0.86f, 0.99f); // Updated from ComboBg
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    // ImGuiCol_Column, ImGuiCol_ColumnHovered, ImGuiCol_ColumnActive are deprecated, use ImGuiCol_Separator, ImGuiCol_SeparatorHovered, ImGuiCol_SeparatorActive instead
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    // ImGuiCol_CloseButton, ImGuiCol_CloseButtonHovered, ImGuiCol_CloseButtonActive are deprecated and removed, so skip them
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    // ImGuiCol_ModalWindowDarkening is deprecated, use ImGuiCol_ModalWindowDimBg instead
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

        // Define style control variables
        bool bStyleDark_ = true; // Set to true for dark variant
        float alpha_ = 1.0f;      // Transparency factor

        // Apply dark variant or alpha adjustments
        if (bStyleDark_) {
            for (int i = 0; i <= ImGuiCol_COUNT; i++) {
                ImVec4& col = style.Colors[i];
                float H, S, V;
                ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);
                if (S < 0.1f) {
                    V = 1.0f - V;
                }
                ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
                if (col.w < 1.00f) {
                    col.w *= alpha_;
                }
            }
        } else {
            for (int i = 0; i <= ImGuiCol_COUNT; i++) {
                ImVec4& col = style.Colors[i];
                if (col.w < 1.00f) {
                    col.x *= alpha_;
                    col.y *= alpha_;
                    col.z *= alpha_;
                    col.w *= alpha_;
                }
            }
        }


      }


void UI::cleanup() {}

void UI::render() {
    using namespace Canvas;
    using namespace Tools;
    using Canvas::Layer;

    // --- Main Menu Bar ---
    if (ImGui::BeginMainMenuBar()) {
        // --- File Menu ---
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
        // --- Edit Menu ---
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) Editor::applyUndo();
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) Editor::applyRedo();
            ImGui::Separator();
            if (ImGui::MenuItem("Copy", "Ctrl+C", false, ::hasSelection)) Editor::copySelection();
            if (ImGui::MenuItem("Paste", "Ctrl+V")) Editor::pasteSelection();
            if (ImGui::MenuItem("Clear Selection", "Delete", false, ::hasSelection)) Editor::deleteSelection();
            ImGui::EndMenu();
        }
        // --- Image Menu ---
        if (ImGui::BeginMenu("Image")) {
            if (ImGui::MenuItem("Crop", nullptr, false, hasSelection)) {
                Canvas::cropImage();
            }
            if (ImGui::MenuItem("Rotate 90° CW")) {
                Canvas::rotateImage(90);
            }
            if (ImGui::MenuItem("Rotate 180°")) {
                Canvas::rotateImage(180);
            }
            if (ImGui::MenuItem("Rotate 270° CCW")) {
                Canvas::rotateImage(270);
            }
            if (ImGui::MenuItem("Resize...")) {
                showResizeDialog = true;
            }
            ImGui::EndMenu();
        }
        // --- Filters Menu ---
        if (ImGui::BeginMenu("Filters")) {
            if (ImGui::MenuItem("Grayscale")) {
                Canvas::applyGrayscale();
            }
            if (ImGui::MenuItem("Blur...")) {
                showBlurDialog = true;
            }
            if (ImGui::MenuItem("Adjust Contrast")) {
                showContrastDialog = true;
            }
            if (ImGui::MenuItem("Adjust Hue/Saturation...")) {
                showHueSaturationDialog = true;
            }
            ImGui::EndMenu();
        }
        // --- Help Menu ---
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) showAboutDialog = true;
            if (ImGui::MenuItem("Keyboard Shortcuts", "H")) showHelpDialog = true;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // --- Resize Dialog ---
    if (showResizeDialog) {
        ImGui::OpenPopup("Resize Image");
        showResizeDialog = false;
    }
    if (ImGui::BeginPopupModal("Resize Image", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static int newWidth = 0;
        static int newHeight = 0;
        if (newWidth == 0 || newHeight == 0) {
            newWidth = CANVAS_WIDTH;
            newHeight = CANVAS_HEIGHT;
        }
        ImGui::InputInt("Width", &newWidth);
        ImGui::InputInt("Height", &newHeight);
        newWidth = std::max(1, newWidth);
        newHeight = std::max(1, newHeight);
        ImGui::TextWrapped("Note: This resizes the entire canvas and all layers.");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            resizeCanvas(newWidth, newHeight);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // --- Contrast Dialog ---
    if (showContrastDialog) {
        ImGui::OpenPopup("Adjust Contrast");
        showContrastDialog = false;
    }
    if (ImGui::BeginPopupModal("Adjust Contrast", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static float contrastValue = 128.0f;
        ImGui::SliderFloat("Contrast", &contrastValue, 0.0f, 255.0f, "%.1f");
        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            Canvas::adjustContrast(contrastValue);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // --- About Dialog ---
    if (showAboutDialog) {
        ImGui::OpenPopup("About");
        showAboutDialog = false;
    }
    if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enough Image Editor");
        ImGui::Text("Version 1.0.2");
        ImGui::Text("Created with SDL2, Dear ImGui, and TinyFileDialogs");
        ImGui::Text("This is a simple image editor with basic tools and filters.");
        ImGui::Text("For more information, visit the GitHub repository:");
        ImGui::Text("This is affectionate and nostalgic memory of a certain person and their antics. To paint a kinder world for that spirit to forever exist in.");
        ImGui::Separator();
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // --- Shortcuts Dialog ---
    if (showHelpDialog) {
        ImGui::OpenPopup("Shortcuts");
        showHelpDialog = false;
    }
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

    // --- Tools & Properties Panel ---
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
        currentTool == GRADIENT ? "Gradient" :
        currentTool == HEALING ? "Healing" : "None");

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

    if (ImGui::Button("Healing Brush (H)", ImVec2(120, 30))) currentTool = HEALING;
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

    if (resizeCorner == -1) {
        Canvas::drawResizeHandles(renderer);
    }
    ImGui::End();

    // --- Text Box Editor Window ---
    const auto& textBoxes = Tools::get_text_boxes();
    int activeTextBox = Tools::getActiveTextBoxIndex();
    if (activeTextBox >= 0 && activeTextBox < (int)textBoxes.size()) {
        ImGui::SetNextWindowSize(ImVec2(350, 250), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(CANVAS_WIDTH + 50, 300), ImGuiCond_FirstUseEver);

        bool showEditor = true;
        if (ImGui::Begin("Text Editor", &showEditor)) {
            const_cast<TextBox&>(textBoxes[activeTextBox]); // If you need to edit
            TextBox& textBox = const_cast<TextBox&>(textBoxes[activeTextBox]);

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

            // Position and size
            ImGui::Text("Position & Size");
            ImGui::SliderInt("X", &textBox.rect.x, 0, CANVAS_WIDTH - textBox.rect.w);
            ImGui::SliderInt("Y", &textBox.rect.y, 0, CANVAS_HEIGHT - textBox.rect.h);
            ImGui::SliderInt("Width", &textBox.rect.w, 50, CANVAS_WIDTH);
            ImGui::SliderInt("Height", &textBox.rect.h, 20, CANVAS_HEIGHT);

            ImGui::Separator();

            if (ImGui::Button("Done")) {
                Tools::finalizeTextBox(activeTextBox);
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                Tools::deleteTextBox(activeTextBox);
            }
        }
        ImGui::End();

        if (!showEditor) {
            Tools::finalizeTextBox(activeTextBox);
        }
    }

    // --- Blur Dialog ---
    if (showBlurDialog) {
        ImGui::OpenPopup("Blur");
        showBlurDialog = false;
    }
    if (ImGui::BeginPopupModal("Blur", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static int blurStrength = 2;
        ImGui::SliderInt("Strength", &blurStrength, 1, 10);
        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            Canvas::applyBlur(blurStrength);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // --- Hue/Saturation Dialog ---
    if (showHueSaturationDialog) {
        ImGui::OpenPopup("Adjust Hue/Saturation");
        showHueSaturationDialog = false;
    }
    if (ImGui::BeginPopupModal("Adjust Hue/Saturation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static float saturationValue = 0.0f; // Range: -1.0 to 1.0
        ImGui::SliderFloat("Saturation", &saturationValue, -1.0f, 1.0f, "%.2f");
        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            Canvas::applyAdjustment(ADJUST_HUE_SATURATION, saturationValue);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}