#include "UI.hpp"
#include "../canvas/Canvas.hpp"
#include "../tools/Tool.hpp"
#include "../editor/Editor.hpp"
#include "../canvas/Layer.hpp"
#include "../imgui/imgui.h"
#include "../tinyfiledialogs/tinyfiledialogs.h"
#include <cstring>
#include <algorithm>
#include <iostream>
#include "../imgui/imgui_impl_sdl2.h"
#include "../imgui/imgui_impl_sdlrenderer2.h"


UI& UI::getInstance() {
    static UI instance;
    return instance;
}

UI::UI()
    : m_showNewCanvasDialog(false),
      m_showResizeDialog(false),
      m_showContrastDialog(false),
      m_showHueSaturationDialog(false),
      m_showBrightnessDialog(false),
      m_showGammaDialog(false),
      m_showBlurDialog(false),
      m_showHelpDialog(false),
      m_showAboutDialog(false),
      m_newCanvasWidth(1280),
      m_newCanvasHeight(720),
      m_contrastValue(0.0f),
      m_saturationValue(0.0f),
      m_brightnessValue(0.0f),
      m_gammaValue(0.0f),
      m_blurStrength(1) {
}

bool UI::init(SDL_Window* window, SDL_Renderer* renderer) {
    if (m_initialized) {
        return true;
    }

    IMGUI_CHECKVERSION();

    if (ImGui::GetCurrentContext() == nullptr) {
        ImGui::CreateContext();
    }

    ImGuiIO& io = ImGui::GetIO();

    if (io.BackendPlatformUserData != nullptr) {
        ImGui_ImplSDL2_Shutdown();
    }

    setupTheme();

    if (!ImGui_ImplSDL2_InitForSDLRenderer(window, renderer)) {
        std::cerr << "ImGui SDL2 init failed" << std::endl;
        return false;
    }

    if (!ImGui_ImplSDLRenderer2_Init(renderer)) {
        std::cerr << "ImGui renderer init failed" << std::endl;
        ImGui_ImplSDL2_Shutdown();
        return false;
    }

    m_initialized = true;
    return true;
}

void UI::setupTheme() {
    // credit goes to this guy -> https://github.com/GraphicsProgramming/dear-imgui-styles
    // I am using the dougblinks style
    // Video on youtube helped me learn about imgui theming
    // https://youtu.be/790aMkbsBm8?si=dsrvEpvLPfqVULAp by Skyed
    ImGui::CreateContext();
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.FrameRounding = 3.0f;
    style.Colors[ImGuiCol_Text]                  = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
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
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

    for (int i = 0; i <= ImGuiCol_COUNT; i++) {
        ImVec4& col = style.Colors[i];
        float H, S, V;
        ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);
        if (S < 0.1f) {
            V = 1.0f - V;
        }
        ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
        if (col.w < 1.00f) {
            col.w *= 1.0f;
        }
    }
}

void UI::cleanup() {
    if (m_initialized) {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        m_initialized = false;
    }
}

void UI::render() {
    ToolManager& toolManager = GetToolManager();

    const int sidebarWidth = std::max(300, (int)(ImGui::GetIO().DisplaySize.x * 0.2f));

    const float canvasWidth = ImGui::GetIO().DisplaySize.x - sidebarWidth;
    const float windowHeight = ImGui::GetIO().DisplaySize.y;

    renderMenuBar();

    ImGui::SetNextWindowPos(ImVec2(canvasWidth, 0));
    ImGui::SetNextWindowSize(ImVec2(sidebarWidth, windowHeight * 0.25f));
    ImGui::Begin("Tools", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse);
    renderToolPanel();
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(canvasWidth, windowHeight * 0.25f));
    ImGui::SetNextWindowSize(ImVec2(sidebarWidth, windowHeight * 0.25f));
    ImGui::Begin("Colors", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse);
    renderColorPicker();
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(canvasWidth, windowHeight * 0.5f));
    ImGui::SetNextWindowSize(ImVec2(sidebarWidth, windowHeight * 0.3f));
    ImGui::Begin("Layers", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse);
    renderLayerPanel();
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(canvasWidth, windowHeight * 0.8f));
    ImGui::SetNextWindowSize(ImVec2(sidebarWidth, windowHeight * 0.2f));
    ImGui::Begin("Tool Properties", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse);
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), true);
    renderToolProperties();
    ImGui::EndChild();
    ImGui::End();
    if (m_showNewCanvasDialog) renderNewCanvasDialog();
    if (m_showResizeDialog) renderResizeDialog();
    if (m_showContrastDialog) renderContrastDialog();
    if (m_showHueSaturationDialog) renderHueSaturationDialog();
    if (m_showBrightnessDialog) renderBrightnessDialog();
    if (m_showGammaDialog) renderGammaDialog();
    if (m_showBlurDialog) renderBlurDialog();
    if (m_showDirectionalBlurDialog) renderDirectionalBlurDialog();
    if (m_showShadowsHighlightsDialog) renderShadowsHighlightsDialog();
    if (m_showColorBalanceDialog) renderColorBalanceDialog();
    if (m_showCurvesDialog) renderCurvesDialog();
    if (m_showVibranceDialog) renderVibranceDialog();
    if (m_showHelpDialog) renderHelpDialog();
    if (m_showAboutDialog) {
        ImGui::OpenPopup("About");
        m_showAboutDialog = false;
    }
    renderAboutDialog();

    if (toolManager.getCurrentToolIndex() == 9) {
        renderTextEditorModal();
    }

    if (toolManager.getCurrentToolIndex() == 10) {
        ImGui::SetNextWindowPos(ImVec2(canvasWidth + 10, windowHeight * 0.6f));
        ImGui::SetNextWindowSize(ImVec2(sidebarWidth - 20, windowHeight * 0.15f));
        ImGui::Begin("Gradient Properties", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoCollapse);
        renderGradientProperties();
        ImGui::End();
    }
}

void UI::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            renderFileMenu();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            renderEditMenu();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Layer")) {
            renderLayerMenu();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Filter")) {
            renderFilterMenu();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            renderHelpMenu();
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void UI::renderFileMenu() {
    Canvas& canvas = GetCanvas();

    if (ImGui::MenuItem("New Canvas", "Ctrl+N")) {
        m_showNewCanvasDialog = true;
    }
    if (ImGui::MenuItem("Open Image", "Ctrl+O")) {
        const char* filters[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp" };
        const char* filePath = tinyfd_openFileDialog(
            "Open Image",
            "",
            4,
            filters,
            "Image Files",
            0
        );
        if (filePath) {
            canvas.importImage(filePath);
        }
    }
    if (ImGui::MenuItem("Save As", "Ctrl+S")) {
        const char* filters[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp" };
        const char* filePath = tinyfd_saveFileDialog(
            "Save Image",
            "image.png",
            4,
            filters,
            "Image Files"
        );
        if (filePath) {
            // Determine format from extension
            std::string path(filePath);
            std::string format = "PNG";
            size_t extPos = path.find_last_of('.');
            if (extPos != std::string::npos) {
                std::string ext = path.substr(extPos + 1);
                for (char& c : ext) c = toupper(c);
                if (ext == "JPG" || ext == "JPEG") format = "JPG";
                else if (ext == "BMP") format = "BMP";
            }
            canvas.exportImage(filePath, format.c_str());
        }
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Exit", "Alt+F4")) {
        SDL_Event quitEvent;
        quitEvent.type = SDL_QUIT;
        SDL_PushEvent(&quitEvent);
    }
}

void UI::renderEditMenu() {
    Editor& editor = GetEditor();

    if (ImGui::MenuItem("Undo", "Ctrl+Z", false, editor.getUndoStackSize() > 0)) {
        editor.applyUndo();
    }
    if (ImGui::MenuItem("Redo", "Ctrl+Y", false, editor.getRedoStackSize() > 0)) {
        editor.applyRedo();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Copy", "Ctrl+C")) {
        editor.copySelection();
    }
    if (ImGui::MenuItem("Paste", "Ctrl+V")) {
        editor.pasteSelection();
    }
    if (ImGui::MenuItem("Delete", "Del")) {
        editor.deleteSelection();
    }
    if (ImGui::MenuItem("Deselect All", "Ctrl+D")) {
        Canvas& canvas = GetCanvas();
        canvas.deselectAll();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Resize Canvas")) {
        m_showResizeDialog = true;
    }
    if (ImGui::MenuItem("Crop to Selection")) {
        Canvas& canvas = GetCanvas();
        if (canvas.hasSelection()) {
            canvas.cropImage();
        }
    }
}

void UI::renderLayerMenu() {
    Canvas& canvas = GetCanvas();
    Editor& editor = GetEditor();

    if (ImGui::MenuItem("Add Layer")) {
        canvas.addLayer();
    }

    if (canvas.getLayers().size() > 1) {
        if (ImGui::MenuItem("Remove Layer")) {
            canvas.removeLayer(canvas.getActiveLayerIndex());
        }
        if (ImGui::MenuItem("Duplicate Layer")) {
            canvas.duplicateLayer(canvas.getActiveLayerIndex());
        }
        if (ImGui::MenuItem("Merge Layers")) {
            editor.mergeLayers();
        }
    }
}

void UI::renderFilterMenu() {
    if (ImGui::MenuItem("Grayscale")) {
        Canvas& canvas = GetCanvas();
        Editor::getInstance().saveUndoState();
        canvas.applyGrayscale();
    }
    if (ImGui::MenuItem("Contrast")) {
        m_showContrastDialog = true;
    }
    if (ImGui::MenuItem("Hue/Saturation")) {
        m_showHueSaturationDialog = true;
    }
    if (ImGui::MenuItem("Brightness")) {
        m_showBrightnessDialog = true;
    }
    if (ImGui::MenuItem("Gamma")) {
        m_showGammaDialog = true;
    }
    if (ImGui::MenuItem("Blur")) {
        m_showBlurDialog = true;
    }
    if (ImGui::MenuItem("Edge Detection")) {
        Canvas& canvas = GetCanvas();
        canvas.applyEdgeDetection();
    }
    if (ImGui::MenuItem("Directional Blur")) {
        m_showDirectionalBlurDialog = true;
    }
    ImGui::Separator();
    if (ImGui::BeginMenu("Color Grading")) {
        if (ImGui::MenuItem("Shadows/Highlights")) {
            m_showShadowsHighlightsDialog = true;
        }
        if (ImGui::MenuItem("Color Balance")) {
            m_showColorBalanceDialog = true;
        }
        if (ImGui::MenuItem("Curves")) {
            m_showCurvesDialog = true;
        }
        if (ImGui::MenuItem("Vibrance")) {
            m_showVibranceDialog = true;
        }
        ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::BeginMenu("Transform")) {
        Canvas& canvas = GetCanvas();
        if (ImGui::MenuItem("Flip Horizontal")) {
            canvas.flipHorizontal(false);
        }
        if (ImGui::MenuItem("Flip Vertical")) {
            canvas.flipVertical(false);
        }
        if (ImGui::MenuItem("Flip Canvas Horizontal")) {
            canvas.flipHorizontal(true);
        }
        if (ImGui::MenuItem("Flip Canvas Vertical")) {
            canvas.flipVertical(true);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("90° Clockwise")) {
            canvas.rotateImage(90);
        }
        if (ImGui::MenuItem("90° Counterclockwise")) {
            canvas.rotateImage(-90);
        }
        if (ImGui::MenuItem("180°")) {
            canvas.rotateImage(180);
        }
        ImGui::EndMenu();
    }
}

void UI::renderHelpMenu() {
    if (ImGui::MenuItem("Help")) {
        m_showHelpDialog = true;
    }
    if (ImGui::MenuItem("About")) {
        m_showAboutDialog = true;
    }
}

void UI::renderToolPanel() {
    ToolManager& toolManager = GetToolManager();

    int currentToolIndex = toolManager.getCurrentToolIndex();
    Tool* currentTool = toolManager.getCurrentTool();
    if (currentTool) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Current: %s", currentTool->getName());
        if (strlen(currentTool->getTooltip()) > 0) {
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted(currentTool->getTooltip());
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }
        ImGui::Separator();
    }

    float panelWidth = ImGui::GetContentRegionAvail().x;
    float buttonWidth = (panelWidth - ImGui::GetStyle().ItemSpacing.x * 2) / 3.0f;
    float buttonHeight = 30.0f;
    ImVec2 buttonSize(buttonWidth, buttonHeight);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));


    if (currentToolIndex == 0) {
        ImGui::Button("Pencil", buttonSize);
    } else {
        ImGui::PopStyleColor(3);
        if (ImGui::Button("Pencil", buttonSize)) toolManager.setCurrentTool(0);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    }
    ImGui::SameLine();

    if (currentToolIndex == 1) {
        ImGui::Button("Eraser", buttonSize);
    } else {
        ImGui::PopStyleColor(3);
        if (ImGui::Button("Eraser", buttonSize)) toolManager.setCurrentTool(1);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    }
    ImGui::SameLine();

    if (currentToolIndex == 2) {
        ImGui::Button("Line", buttonSize);
    } else {
        ImGui::PopStyleColor(3);
        if (ImGui::Button("Line", buttonSize)) toolManager.setCurrentTool(2);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    }


    if (currentToolIndex == 3) {
        ImGui::Button("Rectangle", buttonSize);
    } else {
        ImGui::PopStyleColor(3);
        if (ImGui::Button("Rectangle", buttonSize)) toolManager.setCurrentTool(3);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    }
    ImGui::SameLine();

    if (currentToolIndex == 4) {
        ImGui::Button("Circle", buttonSize);
    } else {
        ImGui::PopStyleColor(3);
        if (ImGui::Button("Circle", buttonSize)) toolManager.setCurrentTool(4);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    }
    ImGui::SameLine();

    if (currentToolIndex == 5) {
        ImGui::Button("Triangle", buttonSize);
    } else {
        ImGui::PopStyleColor(3);
        if (ImGui::Button("Triangle", buttonSize)) toolManager.setCurrentTool(5);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    }


    if (currentToolIndex == 6) {
        ImGui::Button("Fill", buttonSize);
    } else {
        ImGui::PopStyleColor(3);
        if (ImGui::Button("Fill", buttonSize)) toolManager.setCurrentTool(6);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    }
    ImGui::SameLine();

    if (currentToolIndex == 7) {
        ImGui::Button("Select", buttonSize);
    } else {
        ImGui::PopStyleColor(3);
        if (ImGui::Button("Select", buttonSize)) toolManager.setCurrentTool(7);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    }
    ImGui::SameLine();

    if (currentToolIndex == 8) {
        ImGui::Button("FloodSel", buttonSize);
    } else {
        ImGui::PopStyleColor(3);
        if (ImGui::Button("FloodSel", buttonSize)) toolManager.setCurrentTool(8);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    }


    if (currentToolIndex == 9) {
        ImGui::Button("Text", buttonSize);
    } else {
        ImGui::PopStyleColor(3);
        if (ImGui::Button("Text", buttonSize)) toolManager.setCurrentTool(9);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    }
    ImGui::SameLine();

    if (currentToolIndex == 10) {
        ImGui::Button("Gradient", buttonSize);
    } else {
        ImGui::PopStyleColor(3);
        if (ImGui::Button("Gradient", buttonSize)) toolManager.setCurrentTool(10);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    }
    ImGui::SameLine();

    if (currentToolIndex == 11) {
        ImGui::Button("Healing", buttonSize);
    } else {
        ImGui::PopStyleColor(3);
        if (ImGui::Button("Healing", buttonSize)) toolManager.setCurrentTool(11);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    }

    // Clean up the style
    ImGui::PopStyleColor(3);

    ImGui::Separator();

    // Size sliders
    int brushSize = toolManager.getBrushSize();
    if (ImGui::SliderInt("Brush Size", &brushSize, 1, 50)) {
        toolManager.setBrushSize(brushSize);
    }

    int eraserSize = toolManager.getEraserSize();
    if (ImGui::SliderInt("Eraser Size", &eraserSize, 1, 100)) {
        toolManager.setEraserSize(eraserSize);
    }
}

void UI::renderColorPicker() {
    ToolManager& toolManager = GetToolManager();

    // Primary color picker - the main one
    ImVec4 primaryColor = toolManager.getPrimaryColor();
    if (ImGui::ColorPicker4("Primary Color", (float*)&primaryColor)) {
        toolManager.setPrimaryColor(primaryColor);
    }

    // Secondary color for gradients and such
    ImVec4 secondaryColor = toolManager.getSecondaryColor();
    if (ImGui::ColorEdit4("Secondary Color", (float*)&secondaryColor)) {
        toolManager.setSecondaryColor(secondaryColor);
    }

    ImGui::Separator();

    // Swap colors button - handy for quick switching
    if (ImGui::Button("Swap Colors")) {
        ImVec4 temp = primaryColor;
        toolManager.setPrimaryColor(secondaryColor);
        toolManager.setSecondaryColor(temp);
    }
}

void UI::renderLayerPanel() {
    Canvas& canvas = GetCanvas();
    const auto& layers = canvas.getLayers();

    // Display layers in reverse order (top layer first in UI)
    for (int i = layers.size() - 1; i >= 0; i--) {
        const auto& layer = layers[i];

        ImGui::PushID(i);

        bool isActive = (i == canvas.getActiveLayerIndex());

        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));

        std::string layerLabel = ">> " + layer->getName();
        if (ImGui::Selectable(layerLabel.c_str(), isActive, ImGuiSelectableFlags_AllowDoubleClick)) {
            canvas.setActiveLayerIndex(i);
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("LAYER_REORDER", &i, sizeof(int));
            ImGui::Text("Moving: %s", layer->getName().c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LAYER_REORDER")) {
                int sourceLayer = *(const int*)payload->Data;
                if (sourceLayer != i) {
                    canvas.moveLayer(sourceLayer, i);
                    if (canvas.getActiveLayerIndex() == sourceLayer) {
                        canvas.setActiveLayerIndex(i);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::PopStyleVar();

        ImGui::SameLine();

        bool isVisible = layer->isVisible();
        if (isVisible) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
            if (ImGui::SmallButton("Show")) {
                layer->setVisible(false);
            }
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
            if (ImGui::SmallButton("Hide")) {
                layer->setVisible(true);
            }
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();

        bool isLocked = layer->isLocked();
        if (isLocked) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
            if (ImGui::SmallButton("Lock")) {
                layer->setLocked(false);
            }
            ImGui::PopStyleColor();
        } else {
            if (ImGui::SmallButton("Unlock")) {
                layer->setLocked(true);
            }
        }

        ImGui::SameLine();

        float opacity = layer->getOpacity();
        ImGui::PushItemWidth(80);
        if (ImGui::SliderFloat("##opacity", &opacity, 0.0f, 1.0f, "%.2f")) {
            layer->setOpacity(opacity);
        }
        ImGui::PopItemWidth();

        ImGui::Indent(20);
        const char* blendModeItems[] = { "Normal", "Multiply", "Screen", "Overlay", "Darken", "Lighten", "Color Dodge", "Color Burn", "Hard Light", "Soft Light", "Difference", "Exclusion" };
        int blendMode = layer->getBlendMode();

        ImGui::PushItemWidth(140);
        if (ImGui::Combo("##blendmode", &blendMode, blendModeItems, IM_ARRAYSIZE(blendModeItems))) {
            layer->setBlendMode(blendMode);
        }
        ImGui::PopItemWidth();
        ImGui::Unindent(20);

        if (i > 0) {
            ImGui::Separator();
        }

        ImGui::PopID();
    }

    ImGui::Separator();

    if (ImGui::Button("Add Layer")) {
        canvas.addLayer();
    }

    ImGui::SameLine();

    if (ImGui::Button("Remove Layer") && layers.size() > 1) {
        canvas.removeLayer(canvas.getActiveLayerIndex());
    }

    ImGui::SameLine();

    if (ImGui::Button("Duplicate")) {
        canvas.duplicateLayer(canvas.getActiveLayerIndex());
    }
}

void UI::renderTextEditorModal() {
    ToolManager& toolManager = GetToolManager();
    TextTool* textTool = toolManager.getTextTool();

    if (!textTool) return;

    int activeBoxIndex = textTool->getActiveTextBoxIndex();
    if (activeBoxIndex >= 0 && activeBoxIndex < static_cast<int>(textTool->getTextBoxes().size())) {
        ImGui::SetNextWindowSize(ImVec2(420, 350), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 450, 250), ImGuiCond_FirstUseEver);

        bool showEditor = true;
        if (ImGui::Begin("Text Editor - Make Your Words Shine", &showEditor)) {
            TextTool::TextBox* textBox = textTool->getTextBox(activeBoxIndex);

            if (textBox) {
                ImGui::Text("Text:");
                static char textBuffer[512];
                strncpy(textBuffer, textBox->content.c_str(), sizeof(textBuffer) - 1);
                textBuffer[sizeof(textBuffer) - 1] = '\0';

                if (ImGui::InputTextMultiline("##text", textBuffer, sizeof(textBuffer), ImVec2(-1, 100))) {
                    textBox->content = std::string(textBuffer);
                }

                ImGui::Separator();

                ImGui::Text("Font & Style:");

                const auto& availableFonts = textTool->getAvailableFonts();
                if (!availableFonts.empty()) {
                    int currentFontIndex = 0;
                    for (int i = 0; i < static_cast<int>(availableFonts.size()); i++) {
                        if (availableFonts[i] == textBox->fontPath ||
                            (textBox->fontPath.empty() && i == 0)) {
                            currentFontIndex = i;
                            break;
                        }
                    }

                    std::vector<const char*> fontNames;
                    static std::vector<std::string> displayNames;

                    for (int i = 0; i < static_cast<int>(availableFonts.size()); i++) {
                        std::string cleanName;

                        if (i == 0) {
                            cleanName = "Default (Arial)";
                        } else {
                            std::string fontPath = availableFonts[i];
                            size_t lastSlash = fontPath.find_last_of("/\\");
                            if (lastSlash != std::string::npos) {
                                cleanName = fontPath.substr(lastSlash + 1);
                            } else {
                                cleanName = fontPath;
                            }

                            size_t dotPos = cleanName.find_last_of('.');
                            if (dotPos != std::string::npos) {
                                cleanName = cleanName.substr(0, dotPos);
                            }


                            while (!cleanName.empty() && (cleanName[0] == '!' || std::isdigit(cleanName[0]) || cleanName[0] == ' ')) {
                                cleanName = cleanName.substr(1);
                            }

                            if (!cleanName.empty()) {
                                cleanName[0] = std::toupper(cleanName[0]);
                            }
                        }

                        if (displayNames.size() <= static_cast<size_t>(i)) {
                            displayNames.resize(i + 1);
                        }
                        // std::cout << "Font: " << cleanName << std::endl; // debug
                        displayNames[i] = cleanName;
                        fontNames.push_back(displayNames[i].c_str());
                    }

                    if (ImGui::Combo("Font Family", &currentFontIndex, fontNames.data(), fontNames.size())) {
                        textBox->fontPath = availableFonts[currentFontIndex];
                        textBox->fontName = fontNames[currentFontIndex];
                    }
                }

                ImGui::SameLine();
                if (ImGui::Button("Browse...")) {
                    const char* filterPatterns[] = { "*.ttf", "*.otf", "*.TTF", "*.OTF" };
                    const char* fontFile = tinyfd_openFileDialog(
                        "Choose a Font File",
                        "fonts/",
                        4,
                        filterPatterns,
                        "Font Files",
                        0
                    );

                    if (fontFile) {
                        std::string fontPath = fontFile;
                        std::string fontName = fontPath;

                        size_t lastSlash = fontName.find_last_of("/\\");
                        if (lastSlash != std::string::npos) {
                            fontName = fontName.substr(lastSlash + 1);
                        }

                        size_t dotPos = fontName.find_last_of('.');
                        if (dotPos != std::string::npos) {
                            fontName = fontName.substr(0, dotPos);
                        }

                        TTF_Font* testFont = TTF_OpenFont(fontPath.c_str(), 12);
                        if (testFont) {
                            TTF_CloseFont(testFont);

                            textBox->fontPath = fontPath;
                            textBox->fontName = fontName + " (Custom)";

                            bool alreadyExists = false;
                            for (const auto& existing : availableFonts) {
                                if (existing == fontPath) {
                                    alreadyExists = true;
                                    break;
                                }
                            }

                            if (!alreadyExists) {
                                textTool->addCustomFont(fontPath, fontName + " (Custom)");
                            }
                        } else {
                            tinyfd_messageBox("Font Error", "Could not load the selected font file.", "ok", "error", 1);
                        }
                    }
                }

                ImGui::SliderInt("Font Size", &textBox->fontSize, 8, 72);

                ImGui::Checkbox("Bold", &textBox->bold);
                ImGui::SameLine();
                ImGui::Checkbox("Italic", &textBox->italic);

                ImGui::ColorEdit4("Color", (float*)&textBox->color);

                ImGui::Separator();

                ImGui::Text("Position & Size:");
                ImGui::Columns(2, nullptr, false);

                ImGui::SliderInt("X Position", &textBox->rect.x, 0, (int)ImGui::GetIO().DisplaySize.x - textBox->rect.w);
                ImGui::SliderInt("Y Position", &textBox->rect.y, 0, (int)ImGui::GetIO().DisplaySize.y - textBox->rect.h);

                ImGui::NextColumn();

                ImGui::SliderInt("Width", &textBox->rect.w, 50, (int)ImGui::GetIO().DisplaySize.x);
                ImGui::SliderInt("Height", &textBox->rect.h, 20, (int)ImGui::GetIO().DisplaySize.y);

                ImGui::Columns(1);

                ImGui::Separator();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
                if (ImGui::Button("Done", ImVec2(100, 30))) {
                    textTool->finalizeTextBox(activeBoxIndex);
                }
                ImGui::PopStyleColor();

                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button("Delete", ImVec2(100, 30))) {
                    textTool->deleteTextBox(activeBoxIndex);
                }
                ImGui::PopStyleColor();

                ImGui::SameLine();

                static bool showPreview = true;
                ImGui::Checkbox("Live Preview", &showPreview);

                if (showPreview) {
                    ImGui::Separator();
                    ImGui::Text("Preview:");
                    ImGui::BeginChild("PreviewArea", ImVec2(0, 50), true); // magic number, whatever

                    ImGui::PushFont(ImGui::GetFont());
                    ImGui::TextWrapped("%s", textBox->content.c_str());
                    ImGui::PopFont();

                    ImGui::EndChild();
                }
            }
        }
        ImGui::End();

        // Close editor if user clicked X - deactivate instead of delete
        if (!showEditor) {
            textTool->deactivateAllTextBoxes();
        }
    }
}

void UI::renderGradientProperties() {
    ToolManager& toolManager = GetToolManager();
    GradientTool* gradientTool = toolManager.getGradientTool();

    if (!gradientTool) return;

    // Gradient type selection
    static int currentType = static_cast<int>(gradientTool->getGradientType());

    if (ImGui::RadioButton("Linear", &currentType, static_cast<int>(GradientType::LINEAR))) {
        gradientTool->setGradientType(GradientType::LINEAR);
    }

    ImGui::SameLine();

    if (ImGui::RadioButton("Radial", &currentType, static_cast<int>(GradientType::RADIAL))) {
        gradientTool->setGradientType(GradientType::RADIAL);
    }

    ImGui::SameLine();

    if (ImGui::RadioButton("Angular", &currentType, static_cast<int>(GradientType::ANGULAR))) {
        gradientTool->setGradientType(GradientType::ANGULAR);
    }

    // Color preview
    ImGui::Text("Start Color:");
    ImGui::ColorButton("##start_color", gradientTool->getColor(), 0, ImVec2(50, 20)); // hardcoded size, fix later

    ImGui::SameLine();

    ImGui::Text("End Color:");
    ImGui::ColorButton("##end_color", gradientTool->getSecondaryColor(), 0, ImVec2(50, 20));

    ImGui::Text("Drag on canvas to create gradient");
}

void UI::renderNewCanvasDialog() {
    ImGui::SetNextWindowSize(ImVec2(300, 150));
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 150, ImGui::GetIO().DisplaySize.y * 0.5f - 75));

    if (ImGui::Begin("New Canvas", &m_showNewCanvasDialog, ImGuiWindowFlags_NoResize)) {
        ImGui::InputInt("Width", &m_newCanvasWidth, 50);
        ImGui::InputInt("Height", &m_newCanvasHeight, 50);

        // Constrain to reasonable values
        m_newCanvasWidth = std::max(1, std::min(m_newCanvasWidth, 4096));
        m_newCanvasHeight = std::max(1, std::min(m_newCanvasHeight, 4096));

        if (ImGui::Button("Create", ImVec2(120, 0))) {
            Canvas& canvas = GetCanvas();
            canvas.setupNewCanvas(m_newCanvasWidth, m_newCanvasHeight);
            m_showNewCanvasDialog = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_showNewCanvasDialog = false;
        }
    }
    ImGui::End();
}

void UI::renderResizeDialog() {
    Canvas& canvas = GetCanvas();

    ImGui::SetNextWindowSize(ImVec2(300, 150));
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 150, ImGui::GetIO().DisplaySize.y * 0.5f - 75));

    if (ImGui::Begin("Resize Canvas", &m_showResizeDialog, ImGuiWindowFlags_NoResize)) {
        static int newWidth = canvas.getWidth();
        static int newHeight = canvas.getHeight();

        ImGui::InputInt("Width", &newWidth, 50);
        ImGui::InputInt("Height", &newHeight, 50);

        // Constrain to reasonable values
        newWidth = std::max(1, std::min(newWidth, 4096));
        newHeight = std::max(1, std::min(newHeight, 4096));

        if (ImGui::Button("Resize", ImVec2(120, 0))) {
            canvas.resizeCanvas(newWidth, newHeight);
            m_showResizeDialog = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_showResizeDialog = false;
        }
    }
    ImGui::End();
}

void UI::renderContrastDialog() {
    Canvas& canvas = GetCanvas();

    ImGui::SetNextWindowSize(ImVec2(300, 120));
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 150, ImGui::GetIO().DisplaySize.y * 0.5f - 60));

    if (ImGui::Begin("Adjust Contrast", &m_showContrastDialog, ImGuiWindowFlags_NoResize)) {
        ImGui::SliderFloat("Contrast", &m_contrastValue, -1.0f, 1.0f);

        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            canvas.adjustContrast(m_contrastValue);
            m_showContrastDialog = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_showContrastDialog = false;
        }
    }
    ImGui::End();
}

void UI::renderHueSaturationDialog() {
    Canvas& canvas = GetCanvas();

    ImGui::SetNextWindowSize(ImVec2(300, 150));
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 150, ImGui::GetIO().DisplaySize.y * 0.5f - 75));

    if (ImGui::Begin("Hue/Saturation", &m_showHueSaturationDialog, ImGuiWindowFlags_NoResize)) {
        static float hueValue = 0.0f;

        ImGui::SliderFloat("Hue", &hueValue, -180.0f, 180.0f);
        ImGui::SliderFloat("Saturation", &m_saturationValue, -1.0f, 1.0f);

        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            // Apply hue shift (normalized to -0.5 to 0.5 range)
            canvas.applyAdjustment(AdjustmentType::HUE_SATURATION, hueValue / 360.0f);
            m_showHueSaturationDialog = false;
            hueValue = 0.0f;
            m_saturationValue = 0.0f;
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_showHueSaturationDialog = false;
            hueValue = 0.0f;
            m_saturationValue = 0.0f;
        }
    }
    ImGui::End();
}

void UI::renderBrightnessDialog() {
    Canvas& canvas = GetCanvas();

    ImGui::SetNextWindowSize(ImVec2(280, 120));
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 140, ImGui::GetIO().DisplaySize.y * 0.5f - 60));

    if (ImGui::Begin("Brightness", &m_showBrightnessDialog, ImGuiWindowFlags_NoResize)) {
        ImGui::SliderFloat("Brightness", &m_brightnessValue, -1.0f, 1.0f);

        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            canvas.applyAdjustment(AdjustmentType::BRIGHTNESS, m_brightnessValue);
            m_showBrightnessDialog = false;
            m_brightnessValue = 0.0f;
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_showBrightnessDialog = false;
            m_brightnessValue = 0.0f;
        }
    }
    ImGui::End();
}

void UI::renderGammaDialog() {
    Canvas& canvas = GetCanvas();

    ImGui::SetNextWindowSize(ImVec2(280, 120));
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 140, ImGui::GetIO().DisplaySize.y * 0.5f - 60));

    if (ImGui::Begin("Gamma Correction", &m_showGammaDialog, ImGuiWindowFlags_NoResize)) {
        ImGui::SliderFloat("Gamma", &m_gammaValue, -2.0f, 2.0f);

        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            canvas.applyAdjustment(AdjustmentType::GAMMA, m_gammaValue);
            m_showGammaDialog = false;
            m_gammaValue = 0.0f;
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_showGammaDialog = false;
            m_gammaValue = 0.0f;
        }
    }
    ImGui::End();
}

void UI::renderBlurDialog() {
    Canvas& canvas = GetCanvas();

    ImGui::SetNextWindowSize(ImVec2(300, 120));
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 150, ImGui::GetIO().DisplaySize.y * 0.5f - 60));

    if (ImGui::Begin("Blur Filter", &m_showBlurDialog, ImGuiWindowFlags_NoResize)) {
        ImGui::SliderInt("Strength", &m_blurStrength, 1, 10);

        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            // WARNING: Using blur then grayscale can cause segfaults
            canvas.applyBlur(m_blurStrength);
            m_showBlurDialog = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_showBlurDialog = false;
        }
    }
    ImGui::End();
}

void UI::renderHelpDialog() {
    ImGui::SetNextWindowSize(ImVec2(500, 300));
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 250, ImGui::GetIO().DisplaySize.y * 0.5f - 150));

    if (ImGui::Begin("Help", &m_showHelpDialog, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Enough Image Editor Help");
        ImGui::Separator();

        ImGui::Text("Tools:");
        ImGui::BulletText("Pencil: Draw freehand lines");
        ImGui::BulletText("Eraser: Erase parts of the image");
        ImGui::BulletText("Line: Draw straight lines");
        ImGui::BulletText("Rectangle: Draw rectangles (filled or outline)");
        ImGui::BulletText("Circle: Draw circles (filled or outline)");
        ImGui::BulletText("Triangle: Draw triangles with consistent line thickness");
        ImGui::BulletText("Fill: Fill areas with a solid color");
        ImGui::BulletText("Text: Add text to the image");
        ImGui::BulletText("Selection: Select a region of the image");
        ImGui::BulletText("Gradient: Create color gradients");
        ImGui::BulletText("Healing: Healing brush for touch-ups");

        ImGui::Separator();

        ImGui::Text("Keyboard Shortcuts:");
        ImGui::BulletText("Ctrl+Z: Undo");
        ImGui::BulletText("Ctrl+Y: Redo");
        ImGui::BulletText("Ctrl+C: Copy selection");
        ImGui::BulletText("Ctrl+V: Paste selection");
        ImGui::BulletText("Ctrl+D: Deselect all");
        ImGui::BulletText("Delete: Delete selection");

        if (ImGui::Button("Close", ImVec2(120, 0))) {
            m_showHelpDialog = false;
        }
    }
    ImGui::End();
}

void UI::renderAboutDialog() {
    if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enough Image Editor");
        ImGui::Text("Version 1.0.2");
        ImGui::Text("Created with SDL2, Dear ImGui, and TinyFileDialogs");
        ImGui::Text("This is a simple image editor with basic tools and filters.");
        ImGui::Text("For more information, visit the GitHub repository:");
        ImGui::Text("This is in affectionate and nostalgic memory of a certain person and their antics. To paint a kinder world for that spirit to forever exist in.");

        ImGui::Separator();
        ImGui::Text("Available Tools:");
        ImGui::BulletText("Pencil - Draw freehand lines with different brush types");
        ImGui::BulletText("Eraser - Remove parts of the image");
        ImGui::BulletText("Line - Draw straight lines with variation");
        ImGui::BulletText("Rectangle - Draw rectangles (filled or outline)");
        ImGui::BulletText("Circle - Draw circles (filled or outline)");
        ImGui::BulletText("Triangle - Draw triangles");
        ImGui::BulletText("Fill - Flood fill areas with color");
        ImGui::BulletText("Select - Select and transform objects");
        ImGui::BulletText("FloodSel - Magic wand selection by color similarity");
        ImGui::BulletText("Text - Add text with custom fonts and styling");
        ImGui::BulletText("Gradient - Create color gradients (linear, radial, angular)");
        ImGui::BulletText("Healing - Touch-up brush for corrections");

        ImGui::Separator();
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void UI::renderDirectionalBlurDialog() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 150, ImGui::GetIO().DisplaySize.y * 0.5f - 100));
    ImGui::SetNextWindowSize(ImVec2(300, 200));

    if (ImGui::Begin("Directional Blur", &m_showDirectionalBlurDialog, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Apply motion blur in a specific direction");
        ImGui::Separator();

        ImGui::SliderInt("Angle", &m_directionalBlurAngle, 0, 359);
        ImGui::SliderInt("Distance", &m_directionalBlurDistance, 1, 20);

        Canvas& canvas = GetCanvas();
        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            // WARNING: Using blur then grayscale can cause segfaults
            canvas.applyDirectionalBlur(m_directionalBlurAngle, m_directionalBlurDistance);
            m_showDirectionalBlurDialog = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_showDirectionalBlurDialog = false;
        }
    }
    ImGui::End();
}

void UI::renderShadowsHighlightsDialog() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 150, ImGui::GetIO().DisplaySize.y * 0.5f - 100));
    ImGui::SetNextWindowSize(ImVec2(300, 200));

    if (ImGui::Begin("Shadows/Highlights", &m_showShadowsHighlightsDialog, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Adjust shadows and highlights separately");
        ImGui::Separator();

        ImGui::SliderFloat("Shadows", &m_shadowsValue, -1.0f, 1.0f);
        ImGui::SliderFloat("Highlights", &m_highlightsValue, -1.0f, 1.0f);

        Canvas& canvas = GetCanvas();
        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            canvas.applyShadowsHighlights(m_shadowsValue, m_highlightsValue);
            m_showShadowsHighlightsDialog = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_showShadowsHighlightsDialog = false;
        }
    }
    ImGui::End();
}

void UI::renderColorBalanceDialog() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 150, ImGui::GetIO().DisplaySize.y * 0.5f - 100));
    ImGui::SetNextWindowSize(ImVec2(300, 240));

    if (ImGui::Begin("Color Balance", &m_showColorBalanceDialog, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Adjust color balance for each channel");
        ImGui::Separator();

        ImGui::SliderFloat("Red", &m_colorBalanceR, -1.0f, 1.0f);
        ImGui::SliderFloat("Green", &m_colorBalanceG, -1.0f, 1.0f);
        ImGui::SliderFloat("Blue", &m_colorBalanceB, -1.0f, 1.0f);

        Canvas& canvas = GetCanvas();
        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            canvas.applyColorBalance(m_colorBalanceR, m_colorBalanceG, m_colorBalanceB);
            m_showColorBalanceDialog = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_showColorBalanceDialog = false;
        }
    }
    ImGui::End();
}

void UI::renderCurvesDialog() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 150, ImGui::GetIO().DisplaySize.y * 0.5f - 100));
    ImGui::SetNextWindowSize(ImVec2(300, 200));

    if (ImGui::Begin("Curves", &m_showCurvesDialog, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Basic curve adjustment (simplified)");
        ImGui::Separator();

        ImGui::SliderFloat("Input", &m_curvesInput, 0.0f, 1.0f);
        ImGui::SliderFloat("Output", &m_curvesOutput, 0.0f, 1.0f);

        Canvas& canvas = GetCanvas();
        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            canvas.applyCurves(m_curvesInput, m_curvesOutput);
            m_showCurvesDialog = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_showCurvesDialog = false;
        }
    }
    ImGui::End();
}

void UI::renderVibranceDialog() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 150, ImGui::GetIO().DisplaySize.y * 0.5f - 100));
    ImGui::SetNextWindowSize(ImVec2(300, 180));

    if (ImGui::Begin("Vibrance", &m_showVibranceDialog, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Enhance color vibrance (smart saturation)");
        ImGui::Separator();

        ImGui::SliderFloat("Vibrance", &m_vibranceValue, -1.0f, 1.0f);

        Canvas& canvas = GetCanvas();
        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            canvas.applyVibrance(m_vibranceValue);
            m_showVibranceDialog = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_showVibranceDialog = false;
        }
    }
    ImGui::End();
}

void UI::renderToolProperties() {
    ToolManager& toolManager = GetToolManager();
    Tool* currentTool = toolManager.getCurrentTool();

    if (!currentTool) {
        ImGui::Text("No tool selected");
        return;
    }

    int currentToolIndex = toolManager.getCurrentToolIndex();

    // Pencil tool properties - brush types
    if (currentToolIndex == 0) {  // Pencil tool
        PencilTool* pencilTool = static_cast<PencilTool*>(currentTool);
        ImGui::Text("Brush Type:");

        int brushType = pencilTool->getBrushType();
        const char* brushNames[] = {"Normal", "Textured", "Soft"};

        if (ImGui::Combo("##BrushType", &brushType, brushNames, 3)) {
            pencilTool->setBrushType(brushType);
        }

        // Show brush description based on type
        switch (brushType) {
            case 0: ImGui::TextDisabled("Solid circle brush"); break;
            case 1: ImGui::TextDisabled("Random dot texture"); break;
            case 2: ImGui::TextDisabled("Soft gradient falloff"); break;
        }
    }

    // Line tool properties - line count
    else if (currentToolIndex == 2) {  // Line tool
        LineTool* lineTool = static_cast<LineTool*>(currentTool);
        ImGui::Text("Line Count:");

        int lineCount = lineTool->getLineCount();
        if (ImGui::SliderInt("##LineCount", &lineCount, 1, 10)) {
            lineTool->setLineCount(lineCount);
        }

        if (lineCount > 1) {
            ImGui::TextDisabled("Multiple lines with variation");
        } else {
            ImGui::TextDisabled("Single line");
        }
    }

    // Flood selection tool properties - color tolerance
    else if (currentToolIndex == 8) {  // FloodSel tool
        FloodSelectionTool* floodTool = static_cast<FloodSelectionTool*>(currentTool);
        ImGui::Text("Color Tolerance:");

        int tolerance = floodTool->getTolerance();
        if (ImGui::SliderInt("##Tolerance", &tolerance, 0, 100)) {
            floodTool->setTolerance(tolerance);
        }

        // Show helpful description based on tolerance level
        if (tolerance < 10) {
            ImGui::TextDisabled("Very precise - similar colors only");
        } else if (tolerance < 30) {
            ImGui::TextDisabled("Moderate - nearby colors");
        } else {
            ImGui::TextDisabled("Loose - wide color range");
        }
    }

    // Gradient tool properties - secondary color and type
    else if (currentToolIndex == 10) {  // Gradient tool (reordered)
        GradientTool* gradientTool = static_cast<GradientTool*>(currentTool);

        ImGui::Text("Secondary Color:");
        ImVec4 secondaryColor = gradientTool->getSecondaryColor();
        if (ImGui::ColorEdit4("##SecondaryColor", (float*)&secondaryColor)) {
            gradientTool->setSecondaryColor(secondaryColor);
        }

        ImGui::Text("Gradient Type:");
        int gradientType = static_cast<int>(gradientTool->getGradientType());
        const char* gradientTypes[] = {"Linear", "Radial", "Angular"};

        if (ImGui::Combo("##GradientType", &gradientType, gradientTypes, 3)) {
            gradientTool->setGradientType(static_cast<GradientType>(gradientType));
        }
    }

    else {
        ImGui::Text("Tool: %s", currentTool->getName());
        if (strlen(currentTool->getTooltip()) > 0) {
            ImGui::TextDisabled("%s", currentTool->getTooltip());
        }
    }
}
