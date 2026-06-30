#pragma once
// このファイルの役割:
// アプリ全体の状態を保持し、UI描画、作画入力、保存・読み込み、Undo/Redoを束ねる。
// SDL初期化やImGui初期化は main.cpp に残し、このクラスはアプリ内部の責務だけを持つ。
// Phase 1.5 Step 20: Undo/RedoをProject全体コピーからアクティブFrame単位へ軽量化する。

#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "core/Frame.h"
#include "core/Project.h"
#include "core/ScenePlate.h"
#include "core/Stroke.h"
#include "core/Timesheet.h"
#include "brush/MyPaintBrushEngine.h"
#include "render/CanvasRenderer.h"
#include "ui/panels/BrushPanel.h"
#include "ui/panels/ColorPanel.h"
#include "ui/panels/ExportPanel.h"
#include "ui/panels/TimesheetPanel.h"

namespace perapera {

enum class AppMode : int {
    Project = 0,
    Storyboard,
    Previs,
    Drawing,
    Coloring,
    Shooting,
    Export,
};

class App {
public:
    App();

    void setRenderer(SDL_Renderer* renderer);
    void draw();

private:
    struct FrameSnapshot {
        int cellIndex = 0;
        int frameIndex = 0;
        Frame frame;
    };

    SDL_Renderer* renderer_ = nullptr;
    Project project_;
    CanvasRenderer canvasRenderer_;

    // libmypaintブラシエンジン（逐次処理用）。
    // PERAPERA_HAS_LIBMYPAINTが未定義の場合はSimple互換として動作する。
    MyPaintBrushEngine myPaintEngine_;
    CanvasView canvasView_;
    AppMode currentMode_ = AppMode::Project;

    int activeCellIndex_ = 0;
    int activeFrameIndex_ = 0;
    int activeLayerIndex_ = 0;

    ui::BrushSettings brushSettings_;
    ui::ColorPanelState colorPanelState_;
    ui::ExportPanelState exportState_;

    // Timesheet Rebuild Step 5:
    // タイムシートUIの一時入力を正式core Timesheetへ同期するためのApp内一時状態。
    // まだProject保存、キャンバス表示、再生、出力には接続しない。
    ui::TimesheetPanelState timesheetPanelState_;
    Timesheet workingTimesheet_;
    bool workingTimesheetDirty_ = false;

    // Timesheet Rebuild Step 7.12:
    // 絵コンテ、レイアウト、仮背景、完成背景をセル列とは別に持つScene Plateの
    // 表示/編集UI用一時状態。Project保存との正式接続は後続Stepで行う。
    ScenePlateStack workingScenePlates_;
    bool workingScenePlatesDirty_ = false;
    int selectedScenePlateIndex_ = 0;

    // Timesheet Rebuild Step 7.13:
    // 画像読み込み前のScene Plateをキャンバス上へダミー矩形として仮表示する。
    // 表示ON/OFF、opacity、zOrder、T範囲、transform の接続確認用。
    bool scenePlateCanvasPreviewEnabled_ = true;

    Stroke currentStroke_;
    bool isDrawingStroke_ = false;
    // MyPaintの逐次処理中かどうかのフラグ。
    // beginStroke〜endStroke の間 true になる。
    bool isMyPaintStrokeActive_ = false;
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

    // Timesheet Rebuild Step 6.5:
    // タイムシートTだけを進める試作再生。activeFrameIndex_ は変更しない。
    bool isPlayingTimesheet_ = false;
    bool timesheetPlaybackPingPong_ = false;
    int timesheetPlaybackDirection_ = 1;
    float timesheetPlaybackSpeed_ = 1.0f;
    float timesheetPlaybackAccumulator_ = 0.0f;

    // Timesheet Rebuild Step 7.9:
    // 任意T範囲だけを再生する。原画間再生は、この範囲指定のプリセットとして扱う。
    bool isPlayingTimesheetRange_ = false;
    int timesheetPlaybackRangeStartFrame_ = 0;
    int timesheetPlaybackRangeEndFrame_ = 0;

    // Timesheet Rebuild Step 7.10:
    // T選択と作画F編集対象がズレても作画できるようにする。
    // true: タイムシートのT/セル選択時に、表示される作画Fへ編集対象を寄せる。
    bool timesheetEditFollowsSelectedT_ = true;

    // Timesheet Rebuild Step 7.10.5:
    // タイムラインで明示的に別の作画Fを選んだ時は、タイムシート由来の表示Fを同時表示しない。
    // true: TS表示Fと編集Fがズレたら、編集中Fだけを表示して作画を優先する。
    // false: 旧7.10互換として、TS表示の上に編集中Fを重ねる。
    bool timesheetPreferEditingFrameWhenMismatch_ = true;
    bool timesheetDrawActiveFrameOverPreview_ = false;
    int previewWarmCursor_ = 0;
    CanvasDisplayMode previewReadyDisplayMode_ = CanvasDisplayMode::Drawing;
    std::vector<char> previewReadyFlags_;
    int previewReadyCount_ = 0;
    int previewReadyScanCursor_ = 0;

    std::vector<FrameSnapshot> undoStack_;
    std::vector<FrameSnapshot> redoStack_;
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
    void enterColoringMode();
    bool selectPaintLayerForColoring(bool createIfMissing);
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
    void warmPlaybackFrameCache();

    void handleCanvasInput(ImVec2 areaMin, ImVec2 areaSize, bool allowDrawingInput);
    void beginStroke(ImVec2 mouseScreen, ImVec2 areaMin, ImVec2 areaSize);
    void updateStroke(ImVec2 mouseScreen, ImVec2 areaMin, ImVec2 areaSize);
    void applyFloodFillAt(ImVec2 mouseScreen, ImVec2 areaMin, ImVec2 areaSize);
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
