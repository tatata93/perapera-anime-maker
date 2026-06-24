// このファイルの役割:
// Appの基本状態、メインレイアウト、モード切り替え、Undo/Redo、保存/書き出しを実装する。

#include "ui/App.h"

#include <algorithm>

#include <imgui.h>

#include "ui/Theme.h"
#include "ui/panels/ExportPanel.h"

namespace perapera {
namespace {

const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

struct ModeInfo {
    const char* tabLabel;
    const char* title;
    const char* description;
};

constexpr std::array<ModeInfo, 6> kModes = {{
    {"1 プロジェクト", "プロジェクトモード", "作品とカットを管理する場所。Phase 1では空パネル。"},
    {"2 絵コンテ", "絵コンテモード", "絵コンテを描き、台詞・アクション・カメラ指示を書く場所。Phase 4で本実装。"},
    {"2.5 プリビズ", "プリビズモード", "3D下敷きや作画補助を扱う場所。Phase 2.5で本実装。"},
    {"3 作画", "作画モード", "Phase 1で実装するメイン作画画面。"},
    {"4 撮影", "撮影モード", "セル重ね、カメラ、撮影効果を扱う場所。Phase 3で本実装。"},
    {"5 出力", "出力モード", "PNG連番やMP4を書き出す場所。Phase 1では右パネルから最小書き出し可能。"},
}};

int modeIndex(AppMode mode)
{
    return static_cast<int>(mode);
}

AppMode modeFromIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(kModes.size())) {
        return AppMode::Project;
    }
    return static_cast<AppMode>(index);
}


} // namespace

App::App()
    : project_(Project::createDefault())
{
    canvasRenderer_.setCanvasSize(project_.canvas.width, project_.canvas.height);

    lastMessage_ = "Phase 1 Step 1-4 stability v11: ready";
}

void App::setRenderer(SDL_Renderer* renderer)
{
    renderer_ = renderer;
    canvasRenderer_.setRenderer(renderer);
    canvasRenderer_.setCanvasSize(project_.canvas.width, project_.canvas.height);
}

Cell* App::activeCell()
{
    if (activeCellIndex_ < 0 || activeCellIndex_ >= static_cast<int>(project_.cells.size())) {
        return nullptr;
    }
    return &project_.cells[static_cast<std::size_t>(activeCellIndex_)];
}

const Cell* App::activeCell() const
{
    if (activeCellIndex_ < 0 || activeCellIndex_ >= static_cast<int>(project_.cells.size())) {
        return nullptr;
    }
    return &project_.cells[static_cast<std::size_t>(activeCellIndex_)];
}

Frame* App::activeFrame()
{
    Cell* cell = activeCell();
    return cell == nullptr ? nullptr : cell->frameOrNull(activeFrameIndex_);
}

const Frame* App::activeFrame() const
{
    const Cell* cell = activeCell();
    return cell == nullptr ? nullptr : cell->frameOrNull(activeFrameIndex_);
}

Layer* App::activeLayer()
{
    Frame* frame = activeFrame();
    return frame == nullptr ? nullptr : frame->activeLayerOrNull(activeLayerIndex_);
}

const Layer* App::activeLayer() const
{
    const Frame* frame = activeFrame();
    return frame == nullptr ? nullptr : frame->activeLayerOrNull(activeLayerIndex_);
}

void App::draw()
{
    drawMainMenuBar();
    drawMainLayout();
}

void App::drawMainMenuBar()
{
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu(u8c(u8"ファイル"))) {
        if (ImGui::MenuItem(u8c(u8"保存"), "Ctrl+S")) {
            saveProject();
        }
        if (ImGui::MenuItem(u8c(u8"開く"))) {
            loadProject();
        }
        ImGui::Separator();
        ImGui::MenuItem(u8c(u8"新規"), nullptr, false, false);
        ImGui::MenuItem(u8c(u8"別名保存"), nullptr, false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(u8c(u8"編集"))) {
        if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
            undo();
        }
        if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
            redo();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(u8c(u8"表示"))) {
        if (ImGui::MenuItem(u8c(u8"全体表示"), "F")) {
            canvasViewInitialized_ = false;
        }
        ImGui::MenuItem(u8c(u8"グリッド表示"), nullptr, false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(u8c(u8"ヘルプ"))) {
        ImGui::TextUnformatted(u8c(u8"ぺらぺらアニメ作り機 Phase 1 Step 1-4"));
        ImGui::TextUnformatted(u8c(u8"作画UIをCanvasRendererへ接続済み。"));
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void App::drawMainLayout()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("MainLayout", nullptr, flags);
    drawModeTabs();
    drawModeWorkspace();
    drawStatusBar();
    ImGui::End();
}

void App::drawModeTabs()
{
    ImGui::BeginChild("ModeTabs", ImVec2(0.0f, 42.0f), true);

    for (int index = 0; index < static_cast<int>(kModes.size()); ++index) {
        if (index > 0) {
            ImGui::SameLine();
        }

        const bool selected = index == modeIndex(currentMode_);
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ui::themeColors().accent);
        }

        if (ImGui::Button(kModes[static_cast<std::size_t>(index)].tabLabel, ImVec2(150.0f, 28.0f))) {
            currentMode_ = modeFromIndex(index);
        }

        if (selected) {
            ImGui::PopStyleColor();
        }
    }

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 170.0f);
    if (ImGui::Button(u8c(u8"レイアウトリセット"), ImVec2(155.0f, 28.0f))) {
        canvasViewInitialized_ = false;
        lastMessage_ = "layout reset";
    }

    ImGui::EndChild();
}

void App::drawModeWorkspace()
{
    if (currentMode_ == AppMode::Drawing) {
        drawDrawingMode();
        return;
    }

    const ModeInfo& mode = kModes[static_cast<std::size_t>(modeIndex(currentMode_))];
    drawPlaceholderMode(mode.title, mode.description);
}

void App::drawPlaceholderMode(const char* title, const char* description)
{
    ImGui::BeginChild("Workspace", ImVec2(0.0f, -28.0f), true);
    ImGui::TextUnformatted(title);
    ImGui::Separator();
    ImGui::TextWrapped("%s", description);
    ImGui::Spacing();
    ImGui::TextUnformatted(u8c(u8"このモードはまだ空です。作画モードでPhase 1の機能を確認してください。"));
    ImGui::EndChild();
}

void App::drawStatusBar()
{
    const Frame* frame = activeFrame();
    const Layer* layer = activeLayer();
    const int frameCount = activeCell() == nullptr ? 0 : static_cast<int>(activeCell()->frames.size());
    const int layerCount = frame == nullptr ? 0 : static_cast<int>(frame->layers.size());

    ImGui::BeginChild("StatusBar", ImVec2(0.0f, 24.0f), true);
    ImGui::Text("fps: %.1f | canvas: %dx%d | zoom: %.0f%% | frame: %d/%d | layer: %d/%d | brush: %.1f px",
                ImGui::GetIO().Framerate,
                project_.canvas.width,
                project_.canvas.height,
                canvasView_.zoom * 100.0f,
                activeFrameIndex_ + 1,
                frameCount,
                activeLayerIndex_ + 1,
                layerCount,
                brushSettings_.radiusPx);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", lastMessage_.c_str());
    (void)layer;
    ImGui::EndChild();
}

void App::pushUndoSnapshot()
{
    undoStack_.push_back(project_);
    if (undoStack_.size() > 32U) {
        undoStack_.erase(undoStack_.begin());
    }
    redoStack_.clear();
}

void App::undo()
{
    if (undoStack_.empty()) {
        lastMessage_ = "undo stack is empty";
        return;
    }

    redoStack_.push_back(project_);
    project_ = undoStack_.back();
    undoStack_.pop_back();
    afterProjectChanged();
    lastMessage_ = "undo";
}

void App::redo()
{
    if (redoStack_.empty()) {
        lastMessage_ = "redo stack is empty";
        return;
    }

    undoStack_.push_back(project_);
    project_ = redoStack_.back();
    redoStack_.pop_back();
    afterProjectChanged();
    lastMessage_ = "redo";
}

void App::afterProjectChanged()
{
    clampSelection();
    canvasRenderer_.setCanvasSize(project_.canvas.width, project_.canvas.height);
    canvasRenderer_.markAllDirty();
}

void App::clampSelection()
{
    if (project_.cells.empty()) {
        project_ = Project::createDefault();
    }

    activeCellIndex_ = std::clamp(activeCellIndex_, 0, static_cast<int>(project_.cells.size()) - 1);
    Cell* cell = activeCell();
    if (cell == nullptr) {
        return;
    }
    if (cell->frames.empty()) {
        cell->frames.push_back(Frame::createDefault(1));
    }
    activeFrameIndex_ = std::clamp(activeFrameIndex_, 0, static_cast<int>(cell->frames.size()) - 1);

    Frame* frame = activeFrame();
    if (frame == nullptr) {
        return;
    }
    if (frame->layers.empty()) {
        frame->layers.push_back(Layer::createDefault(1));
    }
    activeLayerIndex_ = std::clamp(activeLayerIndex_, 0, static_cast<int>(frame->layers.size()) - 1);
}

} // namespace perapera
