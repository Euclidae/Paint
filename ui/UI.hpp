#pragma once
#include <SDL2/SDL.h>
#include "../imgui/imgui.h"
#include "../tools/Tool.hpp"

class Canvas;
class ToolManager;
class Editor;

class UI {
public:
    static UI& getInstance();

    bool init(SDL_Window* window, SDL_Renderer* renderer);
    void cleanup();
    void render();

private:
    UI();
    ~UI() = default;
    UI(const UI&) = delete;
    UI& operator=(const UI&) = delete;


    void setupTheme();

    void renderMenuBar();
    void renderFileMenu();
    void renderEditMenu();
    void renderViewMenu();
    void renderLayerMenu();
    void renderFilterMenu();
    void renderHelpMenu();

    void renderToolPanel();
    void renderToolProperties();
    void renderLayerPanel();

    void renderColorPicker();
    void renderTextEditorModal();
    void renderGradientProperties();

    void renderNewCanvasDialog();
    void renderResizeDialog();
    void renderContrastDialog();
    void renderHueSaturationDialog();
    void renderBrightnessDialog();
    void renderGammaDialog();
    void renderBlurDialog();
    void renderDirectionalBlurDialog();
    void renderShadowsHighlightsDialog();
    void renderColorBalanceDialog();
    void renderCurvesDialog();
    void renderVibranceDialog();
    void renderHelpDialog();
    void renderAboutDialog();

    bool m_initialized = false;
    bool m_showNewCanvasDialog = false;
    bool m_showResizeDialog = false;
    bool m_showContrastDialog = false;
    bool m_showHueSaturationDialog = false;
    bool m_showBrightnessDialog = false;
    bool m_showGammaDialog = false;
    bool m_showBlurDialog = false;
    bool m_showHelpDialog = false;
    bool m_showAboutDialog = false;
    bool m_showDirectionalBlurDialog = false;
    bool m_showShadowsHighlightsDialog = false;
    bool m_showColorBalanceDialog = false;
    bool m_showCurvesDialog = false;
    bool m_showVibranceDialog = false;

    // Dialog values
    int m_newCanvasWidth = 1280;
    int m_newCanvasHeight = 720;
    float m_contrastValue = 0.0f;
    float m_saturationValue = 0.0f;
    float m_brightnessValue = 0.0f;
    float m_gammaValue = 0.0f;
    int m_blurStrength = 1;

    // Color grading values
    int m_directionalBlurAngle = 0;
    int m_directionalBlurDistance = 5;
    float m_shadowsValue = 0.0f;
    float m_highlightsValue = 0.0f;
    float m_colorBalanceR = 0.0f;
    float m_colorBalanceG = 0.0f;
    float m_colorBalanceB = 0.0f;
    float m_curvesInput = 0.5f;
    float m_curvesOutput = 0.5f;
    float m_vibranceValue = 0.0f;
};


[[nodiscard("This is a singleton so it needs to be referenced.")]]inline UI& GetUI() {
    return UI::getInstance();
}
