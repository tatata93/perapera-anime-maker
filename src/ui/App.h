#pragma once

// このファイルの役割:
// アプリ全体の状態を保持し、UI描画、作画入力、保存・読み込み、Undo/Redoを束ねる。
// SDL初期化やImGui初期化は main.cpp に残し、このクラスはアプリ内部の責務だけを持つ。

#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "core/Project.h"
#include "core/Stroke.h"
#include "render/CanvasRenderer.h"
#include "ui/panels/BrushPanel.h"
#include "ui/panels/ColorPanel.h"
#include "ui/panels/ExportPanel.h"

namespace perapera {

enum class AppMode : int {
    Project = 0,
    Storyboard,
    Previs,
    Drawing,
    Shooting,
    Export,
};

class App {
public:
    App();

    void setRenderer(SDL_Renderer* renderer);
    void draw();

private:
    SDL_Renderer* renderer_ = nullptr;
    Project project_;
    CanvasRenderer canvasRenderer_;
    CanvasView canvasView_;
    AppMode currentMode_ = AppMode::Project;

    int activeCellIndex_ = 0;
    int activeFrameIndex_ = 0;
    int activeLayerIndex_ = 0;

    ui::BrushSettings brushSettings_;
    ui::ColorPanelState colorPanelState_;
    ui::ExportPanelState exportState_;

    Stroke currentStroke_;
    bool isDrawingStroke_ = false;
    bool canvasViewInitialized_ = false;
    bool onionPrevious_ = true;
    bool onionNext_ = false;

    bool lightTableEnabled_ = false;
    float lightTableOpacity_ = 0.35f;
    int lightTableColorMode_ = 0;
    std::vector<int> lightTableFrameIndices_;

    bool isPlayingFrames_ = false;
    bool playbackPingPong_ = false;
    bool playbackSkipAssistOverlays_ = true;
    int playbackDirection_ = 1;
    float playbackSpeed_ = 1.0f;
    float playbackAccumulator_ = 0.0f;

    std::vector<Project> undoStack_;
    std::vector<Project> redoStack_;
    std::string lastMessage_ = "Phase 1 Step 1-4: ready";

    Cell* activeCell();
    const Cell* activeCell() const;
    Frame* activeFrame();
    const Frame* activeFrame() const;
    Layer* activeLayer();
    const Layer* activeLayer() const;

    void drawMainMenuBar();
    void drawMainLayout();
    void drawModeTabs();
    void drawModeWorkspace();
    void drawPlaceholderMode(const char* title, const char* description);
    void drawStatusBar();

    void drawDrawingMode();
    void drawCanvasArea(float rightWidth);
    void drawLeftSidebar();
    void drawRightSidebar();
    void drawTimelineArea();
    void drawFingerPlaybackControls();
    void drawLightTableControls();
    void drawLightTableOverlay(ImVec2 areaMin, ImVec2 areaSize, ImDrawList* drawList);

    bool saveColorPalette(const std::filesystem::path& projectFolder, std::string* error) const;
    bool loadColorPalette(const std::filesystem::path& projectFolder, std::string* error);

    void handleFrameShortcuts();
    void updateFramePlayback();
    bool stepActiveFrame(int delta);
    bool advancePlaybackFrame();

    void handleCanvasInput(ImVec2 areaMin, ImVec2 areaSize);
    void beginStroke(ImVec2 mouseScreen, ImVec2 areaMin, ImVec2 areaSize);
    void updateStroke(ImVec2 mouseScreen, ImVec2 areaMin, ImVec2 areaSize);
    void finishStroke();
    void cancelStroke();
    void fitCanvasToArea(ImVec2 areaSize);
    void removeIntersectingStrokes(const Stroke& eraserStroke);

    void pushUndoSnapshot();
    void undo();
    void redo();
    void afterProjectChanged();
    void clampSelection();

    void addLayer();
    void deleteActiveLayer();
    void clearActiveLayer();
    void addFrame();
    void duplicateFrame();
    void deleteActiveFrame();

    void saveProject();
    void loadProject();
    void saveLoadRoundTripCheck();
    void exportActivePng();
    void exportPngSequence();
    void exportMp4();
};

} // namespace perapera
