#include "ui/App.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <imgui.h>

#include "ui/AppDrawingModeTimesheet.h"
#include "ui/panels/BrushPanel.h"
#include "ui/panels/CellPanel.h"
#include "ui/panels/ColorPanel.h"
#include "ui/panels/ExportPanel.h"
#include "ui/panels/FramePanel.h"
#include "ui/panels/LayerPanel.h"
#include "ui/panels/TimelinePanel.h"

namespace perapera {
namespace {

const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

} // namespace

using app_drawing::buildTimesheetPlaybackOrderFrameIndicesForCell;

void App::drawLeftSidebar()
{
    if (currentMode_ == AppMode::Coloring) {
        ImGui::TextUnformatted(u8c(u8"③.5 彩色モード"));
        ImGui::TextWrapped(u8c(u8"Paintレイヤーを編集対象にし、Normal/ColorTrace/ShadowGuideを参照表示します。"));
        if (ImGui::Button(u8c(u8"Paintレイヤーへ移動"), ImVec2(-1.0f, 0.0f))) {
            selectPaintLayerForColoring(true);
        }
        ImGui::Separator();
    }

    ui::drawBrushPanel(brushSettings_);
    ImGui::Separator();
    ui::drawColorPanel(colorPanelState_, brushSettings_);
    ImGui::Separator();
    ImGui::TextUnformatted(u8c(u8"操作"));
    if (ImGui::Button("Undo", ImVec2(-1.0f, 0.0f))) {
        undo();
    }
    if (ImGui::Button("Redo", ImVec2(-1.0f, 0.0f))) {
        redo();
    }
    if (ImGui::Button(u8c(u8"全体表示"), ImVec2(-1.0f, 0.0f))) {
        canvasViewInitialized_ = false;
    }
    ImGui::TextWrapped(u8c(u8"左ドラッグ: 描く/消す\n中ボタンドラッグ: パン\nホイール: ズーム"));
}
void App::drawRightSidebar()
{
    const ui::CellPanelResult cellPanelResult = ui::drawCellPanel(project_, activeCellIndex_);
    const auto resetPreviewReadiness = [&]() {
        previewWarmCursor_ = activeFrameIndex_;
        previewReadyFlags_.clear();
        previewReadyCount_ = 0;
        previewReadyScanCursor_ = activeFrameIndex_;
    };
    if (cellPanelResult.projectStructureChanged) {
        activeCellIndex_ = cellPanelResult.selectedCellIndex;
        clampSelection();
        resetPreviewReadiness();
        canvasRenderer_.clearLayerCaches();
        lastMessage_ = cellPanelResult.selectionChanged ? "active cell changed" : "cell structure changed";
    } else if (cellPanelResult.selectionChanged) {
        activeCellIndex_ = cellPanelResult.selectedCellIndex;
        clampSelection();
        resetPreviewReadiness();
        canvasRenderer_.markAllDirty();
        lastMessage_ = "active cell changed";
    } else if (cellPanelResult.displayChanged) {
        canvasRenderer_.markAllDirty();
        lastMessage_ = "cell display changed";
    }

    ImGui::Separator();

    Frame* frame = activeFrame();
    Cell* cell = activeCell();

    if (frame == nullptr || cell == nullptr) {
        ImGui::TextUnformatted("No active cell or frame.");
        return;
    }
    ImGui::TextDisabled(currentMode_ == AppMode::Coloring ? "Phase 2-pre CellPanel v1 coloring mode" : "Phase 2-pre CellPanel v1 drawing mode");
    const ui::LayerPanelAction layerAction = ui::drawLayerPanel(*frame, activeLayerIndex_);
    if (layerAction == ui::LayerPanelAction::AddLayer) {
        addLayer();
    } else if (layerAction == ui::LayerPanelAction::DeleteLayer) {
        deleteActiveLayer();
    } else if (layerAction == ui::LayerPanelAction::ClearLayer) {
        clearActiveLayer();
    }
    ImGui::Separator();
    const ui::FramePanelAction frameAction = ui::drawFramePanel(*cell, activeFrameIndex_);
    if (frameAction == ui::FramePanelAction::AddFrame) {
        addFrame();
    } else if (frameAction == ui::FramePanelAction::DuplicateFrame) {
        duplicateFrame();
    } else if (frameAction == ui::FramePanelAction::DeleteFrame) {
        deleteActiveFrame();
    }
    ImGui::Separator();
    const ui::ExportPanelAction exportAction = ui::drawExportPanel(exportState_, lastMessage_.c_str());
    if (exportAction == ui::ExportPanelAction::SaveProject) {
        saveProject();
    } else if (exportAction == ui::ExportPanelAction::LoadProject) {
        loadProject();
    } else if (exportAction == ui::ExportPanelAction::VerifyProjectRoundTrip) {
        saveLoadRoundTripCheck();
    } else if (exportAction == ui::ExportPanelAction::ExportActivePng) {
        exportActivePng();
    } else if (exportAction == ui::ExportPanelAction::ExportPngSequence) {
        exportPngSequence();
    } else if (exportAction == ui::ExportPanelAction::ExportMp4) {
        exportMp4();
    }
}

void App::drawTimelineArea()
{
    Cell* cell = activeCell();
    if (cell == nullptr) {
        return;
    }

    // Timesheet Rebuild Step 7.1:
    // 下部タイムラインは、フレーム列を先に見せる。
    // ライトテーブル/指パラ操作はその下へ移し、視線がフレーム列へ先に行くようにする。
    ImGui::BeginChild("DrawingTimelineFrameStripHost_v26", ImVec2(0.0f, 160.0f), true,
                      ImGuiWindowFlags_NoScrollWithMouse);
    const int prevFrameIndex = activeFrameIndex_;
    const std::vector<int> playbackOrderFrameIndices =
        buildTimesheetPlaybackOrderFrameIndicesForCell(workingTimesheet_, *cell);

    const ui::TimelinePanelAction timelineAction =
        ui::drawTimelinePanel(*cell, activeFrameIndex_, onionPrevious_, onionNext_, playbackOrderFrameIndices);
    if (activeFrameIndex_ != prevFrameIndex) {
        canvasRenderer_.markAllDirty();
    }
    if (timelineAction == ui::TimelinePanelAction::AddFrame) {
        addFrame();
    } else if (timelineAction == ui::TimelinePanelAction::DuplicateFrame) {
        duplicateFrame();
    } else if (timelineAction == ui::TimelinePanelAction::DeleteFrame) {
        deleteActiveFrame();
    }
    ImGui::EndChild();

    ImGui::Separator();
    drawFingerPlaybackControls();
    drawLightTableControls();
}

} // namespace perapera
