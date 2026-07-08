// このファイルの役割:
#include "ui/App.h"
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <imgui.h>
#include "core/TimesheetResolver.h"
#include "core/TimesheetInbetweenResolver.h"
#include "core/TimesheetInbetweenPlanner.h"
#include "core/TimesheetSceneResolver.h"
#include "io/TimesheetIO.h"
#include "ui/AppDrawingModeTimesheet.h"
#include "ui/AppDrawingModeWorkspace.h"
#include "ui/AppProjectIOSupport.h"
#include "ui/Theme.h"
#include "ui/panels/CellPanel.h"
#include "ui/panels/TimesheetPanel.h"
#include "ui/panels/TimesheetPanelBridge.h"
namespace perapera {
using app_drawing::adjacentPlaybackOrderFrameIndex;
using app_drawing::autoCreateMissingDrawingFramesForTimesheetEntries;
using app_drawing::buildTimesheetPlaybackOrderFrameIndicesForCell;
using app_drawing::countTimesheetEntries;
using app_drawing::projectCellById;
using app_drawing::selectedTimesheetPanelEntry;
using app_drawing::timesheetPanelKindUsesDrawingFrame;
namespace {
const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

} // namespace
void App::drawDrawingMode()
{
    // Timesheet Rebuild Step 6:
    // TimesheetPanelの一時入力をApp内の正式core Timesheetへ同期し、
    // T選択をキャンバス表示プレビューへ反映する。
    // まだ再生、出力、Project保存の自動連動には反映しない。
    {
        timesheetPanelState_.showDetachedWindow = true;

        ::perapera::ui::TimesheetPanelViewModel timesheetPanelData =
        ::perapera::app_drawing::buildTimesheetPanelViewModel(project_);

        // Timesheet Rebuild Step 5.6:
        // 現在のセル列・総フレーム数に合わせて、UI一時入力を先に正規化する。
        // セル削除後の古いcellId、範囲外T、重複entryを残したまま保存・変換しない。
        const bool timesheetPanelStateNormalized =
            ::perapera::ui::normalizeTimesheetPanelStateForViewModel(timesheetPanelData, timesheetPanelState_);
        if (timesheetPanelStateNormalized) {
            workingTimesheet_ = ::perapera::ui::buildTimesheetFromPanelState(timesheetPanelData, timesheetPanelState_);
            workingTimesheetDirty_ = true;
            lastMessage_ = "timesheet panel state normalized: entries=" +
                std::to_string(countTimesheetEntries(workingTimesheet_));
        }

        if (workingTimesheet_.tracks.empty() || !workingTimesheetDirty_) {
            workingTimesheet_ = ::perapera::ui::buildTimesheetFromPanelState(timesheetPanelData, timesheetPanelState_);
        } else {
            workingTimesheet_.totalFrames = std::max(1, timesheetPanelData.totalFrames);
        }
        if (isPlayingFrames_ && isPlayingTimesheet_) {
            isPlayingTimesheet_ = false;
            isPlayingTimesheetRange_ = false;
            timesheetPlaybackAccumulator_ = 0.0f;
        }

        // Timesheet Rebuild Step 7.11.5:
        // 増えたタイムシート関連の小ウィンドウを1つにまとめ、作業中の画面を散らかさない。
        ImGui::Begin(
            u8c(u8"タイムシート補助##FormalTimesheetAssistant"),
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
ImGui::TextUnformatted(u8c(u8"Step 7.11.5: タイムシート補助"));
        ImGui::TextDisabled(u8c(u8"再生 / 保存 / T解決結果 / 原画間検出 / 中割作成をここへ集約します。"));
        ImGui::SeparatorText(u8c(u8"T再生 / 作画対象"));
        ImGui::Text(
            u8c(u8"T %d / %d   entries=%d"),
            timesheetPanelState_.selectedTimelineFrame + 1,
            std::max(1, timesheetPanelData.totalFrames),
            countTimesheetEntries(workingTimesheet_));
        ImGui::SeparatorText(u8c(u8"作画対象"));
        ImGui::Checkbox(u8c(u8"T選択で編集対象も追従##TimesheetEditFollowsSelectedT"), &timesheetEditFollowsSelectedT_);
        ImGui::SameLine();
        if (ImGui::SmallButton(u8c(u8"今のTの作画Fを編集##SyncEditToSelectedT"))) {
            if (!syncEditingTargetToSelectedTimesheetCell(timesheetPanelData, "manual timesheet edit sync", true)) {
                lastMessage_ = "manual timesheet edit sync failed: no drawable F for selected T/cell";
            }
        }
        ImGui::Checkbox(u8c(u8"ズレたら編集中Fだけ表示##TimesheetPreferEditingFrameWhenMismatch"), &timesheetPreferEditingFrameWhenMismatch_);
        if (!timesheetPreferEditingFrameWhenMismatch_) {
            ImGui::Checkbox(u8c(u8"旧方式: TS表示の上に編集中Fを重ねる##TimesheetDrawActiveFrameOverPreview"), &timesheetDrawActiveFrameOverPreview_);
        }
        ImGui::TextDisabled(u8c(u8"タイムラインで別の作画Fを選んだ時は、TS由来の作画Fを隠して編集中Fを優先します。"));
        if (ImGui::SmallButton("|<##TimesheetPlaybackFirst")) {
            isPlayingTimesheet_ = false;
            isPlayingTimesheetRange_ = false;
            timesheetPlaybackAccumulator_ = 0.0f;
            setTimesheetPreviewFrame(timesheetPanelData, 0, "timesheet preview");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("<##TimesheetPlaybackPrev")) {
            isPlayingTimesheet_ = false;
            isPlayingTimesheetRange_ = false;
            timesheetPlaybackAccumulator_ = 0.0f;
            stepTimesheetPreviewFrame(timesheetPanelData, -1);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(isPlayingTimesheet_ && !isPlayingTimesheetRange_ ? u8c(u8"停止##TimesheetPlaybackStop") : u8c(u8"全T再生##TimesheetPlaybackPlay"))) {
            if (isPlayingTimesheet_ && !isPlayingTimesheetRange_) {
                isPlayingTimesheet_ = false;
            } else {
                isPlayingTimesheet_ = true;
                isPlayingTimesheetRange_ = false;
            }
            isPlayingFrames_ = false;
            timesheetPlaybackAccumulator_ = 0.0f;
            lastMessage_ = isPlayingTimesheet_ ? "timesheet playback started" : "timesheet playback stopped";
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(">##TimesheetPlaybackNext")) {
            isPlayingTimesheet_ = false;
            isPlayingTimesheetRange_ = false;
            timesheetPlaybackAccumulator_ = 0.0f;
            stepTimesheetPreviewFrame(timesheetPanelData, 1);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(">|##TimesheetPlaybackLast")) {
            isPlayingTimesheet_ = false;
            isPlayingTimesheetRange_ = false;
            timesheetPlaybackAccumulator_ = 0.0f;
            setTimesheetPreviewFrame(timesheetPanelData, timesheetPanelData.totalFrames - 1, "timesheet preview");
        }

        const char* timesheetSpeedItems[] = {"0.25x", "0.5x", "1x", "2x"};
        int timesheetSpeedIndex = 2;
        if (timesheetPlaybackSpeed_ <= 0.26f) {
            timesheetSpeedIndex = 0;
        } else if (timesheetPlaybackSpeed_ <= 0.51f) {
            timesheetSpeedIndex = 1;
        } else if (timesheetPlaybackSpeed_ >= 1.99f) {
            timesheetSpeedIndex = 3;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90.0f);
        if (ImGui::Combo(u8c(u8"速度##TimesheetPlaybackSpeed"), &timesheetSpeedIndex, timesheetSpeedItems, IM_ARRAYSIZE(timesheetSpeedItems))) {
            const float timesheetSpeeds[] = {0.25f, 0.5f, 1.0f, 2.0f};
            timesheetPlaybackSpeed_ = timesheetSpeeds[std::clamp(timesheetSpeedIndex, 0, 3)];
            timesheetPlaybackAccumulator_ = 0.0f;
        }
        ImGui::SameLine();
        ImGui::Checkbox(u8c(u8"ピンポン##TimesheetPlaybackPingPong"), &timesheetPlaybackPingPong_);

        normalizeTimesheetPlaybackRange(timesheetPanelData);
        ImGui::SeparatorText(u8c(u8"T範囲再生"));
        int rangeStartT = timesheetPlaybackRangeStartFrame_ + 1;
        int rangeEndT = timesheetPlaybackRangeEndFrame_ + 1;
        ImGui::SetNextItemWidth(82.0f);
        if (ImGui::InputInt(u8c(u8"開始T##TimesheetRangeStartT"), &rangeStartT)) {
            setTimesheetPlaybackRangeFromOneBased(timesheetPanelData, rangeStartT, rangeEndT, "timesheet range set");
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(82.0f);
        if (ImGui::InputInt(u8c(u8"終了T##TimesheetRangeEndT"), &rangeEndT)) {
            setTimesheetPlaybackRangeFromOneBased(timesheetPanelData, rangeStartT, rangeEndT, "timesheet range set");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(isPlayingTimesheet_ && isPlayingTimesheetRange_ ? u8c(u8"範囲停止##TimesheetRangeStop") : u8c(u8"範囲再生##TimesheetRangePlay"))) {
            normalizeTimesheetPlaybackRange(timesheetPanelData);
            if (isPlayingTimesheet_ && isPlayingTimesheetRange_) {
                isPlayingTimesheet_ = false;
                isPlayingTimesheetRange_ = false;
            } else {
                isPlayingTimesheet_ = true;
                isPlayingTimesheetRange_ = true;
                isPlayingFrames_ = false;
                if (timesheetPanelState_.selectedTimelineFrame < timesheetPlaybackRangeStartFrame_ ||
                    timesheetPanelState_.selectedTimelineFrame > timesheetPlaybackRangeEndFrame_) {
                    setTimesheetPreviewFrame(timesheetPanelData, timesheetPlaybackRangeStartFrame_, "timesheet range playback");
                }
            }
            timesheetPlaybackAccumulator_ = 0.0f;
            lastMessage_ = isPlayingTimesheet_ && isPlayingTimesheetRange_ ? "timesheet range playback started" : "timesheet range playback stopped";
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(u8c(u8"全Tを範囲##TimesheetRangeAll"))) {
            setTimesheetPlaybackRangeFromOneBased(timesheetPanelData, 1, std::max(1, timesheetPanelData.totalFrames), "timesheet range all");
        }
        ImGui::TextDisabled(u8c(u8"タイムシート再生はTだけを進めます。作画F編集対象は変更しません。"));
        ImGui::TextDisabled(u8c(u8"原画間再生は、原画間検出からT範囲へ入れます。"));
        ImGui::TextDisabled(u8c(u8"指パラ/通常タイムライン再生中は、作画F再生表示を優先します。"));
        ImGui::Separator();

        if (isPlayingTimesheet_) {
            if (isDrawingStroke_ || timesheetPanelData.totalFrames <= 1) {
                isPlayingTimesheet_ = false;
                isPlayingTimesheetRange_ = false;
                timesheetPlaybackAccumulator_ = 0.0f;
            } else {
                const float playbackFps = static_cast<float>(std::max(1, timesheetPanelData.fps));
                const float secondsPerTimelineFrame = std::max(1.0f / 60.0f, 1.0f / (playbackFps * std::max(0.01f, timesheetPlaybackSpeed_)));
                timesheetPlaybackAccumulator_ += std::clamp(ImGui::GetIO().DeltaTime, 0.0f, 0.1f);
                while (timesheetPlaybackAccumulator_ >= secondsPerTimelineFrame) {
                    timesheetPlaybackAccumulator_ -= secondsPerTimelineFrame;
                    const int direction = timesheetPlaybackDirection_ == 0 ? 1 : timesheetPlaybackDirection_;
                    if (isPlayingTimesheetRange_) {
                        stepTimesheetRangePreviewFrame(timesheetPanelData, direction);
                    } else {
                        stepTimesheetPreviewFrame(timesheetPanelData, direction);
                    }
                }
            }
        } else {
            timesheetPlaybackAccumulator_ = 0.0f;
        }

        if (!timesheetPanelState_.windowOpen) {
            ImGui::Begin(
                u8c(u8"タイムシートを開く##FormalTimesheetReopen"),
                nullptr,
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::TextUnformatted(u8c(u8"タイムシートは閉じています。"));
            if (ImGui::SmallButton(u8c(u8"タイムシートを開く"))) {
                timesheetPanelState_.windowOpen = true;
            }
            ImGui::End();
        }

        const ::perapera::ui::TimesheetPanelResult timesheetPanelResult =
            ::perapera::ui::drawTimesheetPanel(timesheetPanelData, timesheetPanelState_);

        if (timesheetPanelResult.entryChanged) {
            const int autoCreatedFrames = autoCreateMissingDrawingFramesForTimesheetEntries(project_, timesheetPanelState_);

            const ::perapera::ui::TimesheetPanelEditableEntry* selectedEntry =
                selectedTimesheetPanelEntry(timesheetPanelData, timesheetPanelState_);
            if (selectedEntry != nullptr && timesheetPanelKindUsesDrawingFrame(selectedEntry->kind)) {
                const int safeSelectedCellColumn = std::clamp(
                    timesheetPanelState_.selectedCellColumn,
                    0,
                    static_cast<int>(timesheetPanelData.cells.size()) - 1);
                int selectedProjectCellIndex = -1;
                Cell* selectedCell = projectCellById(
                    project_,
                    timesheetPanelData.cells[static_cast<std::size_t>(safeSelectedCellColumn)].cellId,
                    &selectedProjectCellIndex);
                if (selectedCell != nullptr &&
                    selectedEntry->drawingFrameNumber >= 1 &&
                    selectedEntry->drawingFrameNumber <= static_cast<int>(selectedCell->frames.size())) {
                    activeCellIndex_ = selectedProjectCellIndex;
                    activeFrameIndex_ = selectedEntry->drawingFrameNumber - 1;
                    activeLayerIndex_ = 0;
                }
            }

            workingTimesheet_ = ::perapera::ui::buildTimesheetFromPanelState(timesheetPanelData, timesheetPanelState_);
            workingTimesheetDirty_ = true;
            canvasRenderer_.clearLayerCaches();
            canvasRenderer_.markAllDirty();

            const int entryCount = countTimesheetEntries(workingTimesheet_);
            lastMessage_ = "timesheet temporary core data updated: entries=" + std::to_string(entryCount);
            if (autoCreatedFrames > 0) {
                lastMessage_ += " / auto-created 作画F=" + std::to_string(autoCreatedFrames);
            }
        }

        if (timesheetPanelResult.timelineFrameChanged || timesheetPanelResult.cellSelectionChanged) {
            canvasRenderer_.markAllDirty();
            if (timesheetEditFollowsSelectedT_ &&
                !isPlayingFrames_ &&
                !isPlayingTimesheet_ &&
                !isPlayingTimesheetRange_) {
                if (!syncEditingTargetToSelectedTimesheetCell(timesheetPanelData, "timesheet selection", true)) {
                    lastMessage_ = "timesheet preview T" + std::to_string(timesheetPanelState_.selectedTimelineFrame + 1) +
                        " / edit F" + std::to_string(activeFrameIndex_ + 1) +
                        " / no drawable F to sync";
                }
            } else {
                lastMessage_ = "timesheet preview T" + std::to_string(timesheetPanelState_.selectedTimelineFrame + 1) +
                    " / edit F" + std::to_string(activeFrameIndex_ + 1);
            }
        }

        // Timesheet Rebuild Step 5.5:
        // まだProject保存とは統合せず、現在のプロジェクトフォルダ直下 timesheet.json へ
        // 明示ボタンで一時Timesheetを保存/読み込みする。
        const std::filesystem::path projectFolder = appio::absolutePath(exportState_.projectFolder);
        const std::filesystem::path timesheetPath = TimesheetIO::timesheetPathForCutFolder(projectFolder);

        ImGui::SeparatorText(u8c(u8"保存 / 読み込み"));
        ImGui::TextUnformatted(u8c(u8"一時タイムシートの手動保存/読み込み"));
        ImGui::TextWrapped(
            u8c(u8"保存先: %s"),
            timesheetPath.string().c_str());
        ImGui::Text(
            u8c(u8"一時Timesheet: entries=%d%s"),
            countTimesheetEntries(workingTimesheet_),
            workingTimesheetDirty_ ? u8c(u8" / 未保存の可能性") : u8c(u8""));

        if (ImGui::SmallButton(u8c(u8"一時タイムシート保存"))) {
            workingTimesheet_ = ::perapera::ui::buildTimesheetFromPanelState(timesheetPanelData, timesheetPanelState_);
            workingTimesheet_.totalFrames = std::max(1, timesheetPanelData.totalFrames);

            std::string error;
            if (TimesheetIO::saveTimesheet(workingTimesheet_, timesheetPath, &error)) {
                workingTimesheetDirty_ = false;
                lastMessage_ = "timesheet saved: " + timesheetPath.string() +
                    " | entries=" + std::to_string(countTimesheetEntries(workingTimesheet_));
            } else {
                lastMessage_ = "timesheet save failed: " + error;
            }
        }

        ImGui::SameLine();
        if (ImGui::SmallButton(u8c(u8"一時タイムシート読込"))) {
            Timesheet loadedTimesheet;
            std::string error;
            if (TimesheetIO::loadTimesheet(timesheetPath, loadedTimesheet, &error)) {
                workingTimesheet_ = std::move(loadedTimesheet);
                ::perapera::ui::replacePanelEntriesFromTimesheet(workingTimesheet_, timesheetPanelState_);
                const bool normalizedLoadedPanel =
                    ::perapera::ui::normalizeTimesheetPanelStateForViewModel(timesheetPanelData, timesheetPanelState_);
                if (normalizedLoadedPanel) {
                    workingTimesheet_ = ::perapera::ui::buildTimesheetFromPanelState(timesheetPanelData, timesheetPanelState_);
                }
                workingTimesheetDirty_ = normalizedLoadedPanel;
                lastMessage_ = "timesheet loaded: " + timesheetPath.string() +
                    " | entries=" + std::to_string(countTimesheetEntries(workingTimesheet_));
            } else {
                lastMessage_ = "timesheet load failed: " + error;
            }
        }

        ImGui::TextDisabled(u8c(u8"この保存は試験段階です。Project保存・出力にはまだ正式接続していません。"));
        ImGui::Separator();

        // Timesheet Rebuild Step 7.4:
        // Tを選んだとき、各セルがどの作画Fとして解決されているかを小さい一覧で見せる。
        // 「Tを選んでも何が表示されたのか分からない」を潰すための確認用UI。
        const int resolvedTimelineFrame = clampTimesheetFrame(workingTimesheet_, timesheetPanelState_.selectedTimelineFrame + 1);
        const ui::CellDisplayMode timesheetSummaryDisplayMode = ui::currentCellDisplayMode();
        const int timesheetSummarySoloCellIndex = ui::currentSoloCellIndex();
        std::vector<std::pair<int, const Cell*>> timesheetSummaryCells;

        auto appendTimesheetSummaryCell = [&](int cellIndex, const Cell* cell) {
            if (cell == nullptr) {
                return;
            }
            timesheetSummaryCells.push_back(std::make_pair(cellIndex, cell));
        };

        if (timesheetSummaryDisplayMode == ui::CellDisplayMode::ActiveOnly || project_.cells.empty()) {
            appendTimesheetSummaryCell(activeCellIndex_, activeCell());
        } else if (timesheetSummaryDisplayMode == ui::CellDisplayMode::SoloSelected) {
            const int safeSoloIndex = std::clamp(
                timesheetSummarySoloCellIndex >= 0 ? timesheetSummarySoloCellIndex : activeCellIndex_,
                0,
                static_cast<int>(project_.cells.size()) - 1);
            appendTimesheetSummaryCell(safeSoloIndex, &project_.cells[static_cast<std::size_t>(safeSoloIndex)]);
        } else {
            timesheetSummaryCells.reserve(project_.cells.size());
            for (int cellIndex = 0; cellIndex < static_cast<int>(project_.cells.size()); ++cellIndex) {
                appendTimesheetSummaryCell(cellIndex, &project_.cells[static_cast<std::size_t>(cellIndex)]);
            }
        }

        ImGui::SeparatorText(u8c(u8"タイムシート解決結果"));
        ImGui::Text(
            u8c(u8"T%d の表示結果 / 編集対象: 作画F%d / entries=%d"),
            resolvedTimelineFrame,
            activeFrameIndex_ + 1,
            countTimesheetEntries(workingTimesheet_));
        ImGui::TextDisabled(u8c(u8"T=タイムシート位置 / 作画F=実際に描く絵の番号 / コマ数=時間の長さ"));

        int resolvedVisibleCount = 0;
        if (ImGui::BeginTable(
                "TimesheetResolvedListTable_v27",
                5,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn(u8c(u8"セル"));
            ImGui::TableSetupColumn(u8c(u8"セル状態"));
            ImGui::TableSetupColumn(u8c(u8"記入/保持"));
            ImGui::TableSetupColumn(u8c(u8"表示"));
            ImGui::TableSetupColumn(u8c(u8"備考"));
            ImGui::TableHeadersRow();

            for (const auto& cellPair : timesheetSummaryCells) {
                const int cellIndex = cellPair.first;
                const Cell* cell = cellPair.second;
                if (cell == nullptr) {
                    continue;
                }

                const ResolvedTimesheetCell resolved = resolveTimesheetCell(workingTimesheet_, cell->id, resolvedTimelineFrame);
                const bool cellEnabledForAllDisplay = cell->visible && cell->opacity > 0.0f;
                const bool displayModeKeepsHiddenCell = timesheetSummaryDisplayMode == ui::CellDisplayMode::ActiveOnly ||
                    timesheetSummaryDisplayMode == ui::CellDisplayMode::SoloSelected;
                const bool cellCanAppear = cellEnabledForAllDisplay || displayModeKeepsHiddenCell;
                const bool frameExists = resolved.drawingFrameNumber > 0 &&
                    resolved.drawingFrameNumber <= static_cast<int>(cell->frames.size());
                const bool visibleOnCanvas = cellCanAppear && resolved.visible && frameExists;
                if (visibleOnCanvas) {
                    ++resolvedVisibleCount;
                }

                const char* sourceLabel = resolved.sourceEntry == nullptr
                    ? u8c(u8"未記入")
                    : timesheetEntryTypeJapaneseLabel(resolved.sourceType);
                const std::string cellLabel = cell->name.empty()
                    ? std::string(u8c(u8"セル")) + std::to_string(cellIndex + 1)
                    : cell->name;
                const std::string cellState = cellEnabledForAllDisplay
                    ? std::string(u8c(u8"ON"))
                    : std::string(u8c(u8"セルOFF/不透明度0"));
                std::string displayLabel = u8c(u8"非表示");
                if (visibleOnCanvas) {
                    displayLabel = std::string(u8c(u8"作画F")) + std::to_string(resolved.drawingFrameNumber);
                } else if (resolved.visible && !frameExists) {
                    displayLabel = std::string(u8c(u8"作画F")) + std::to_string(resolved.drawingFrameNumber) + u8c(u8" 範囲外");
                }

                std::string note;
                if (resolved.sourceEntry == nullptr) {
                    note = u8c(u8"このT以前に記入なし");
                } else if (resolved.sourceType == TimesheetEntryType::Hold) {
                    note = u8c(u8"前の表示を保持");
                } else if (resolved.sourceType == TimesheetEntryType::Null) {
                    note = u8c(u8"空セルで非表示");
                } else if (!cellCanAppear) {
                    note = u8c(u8"セル表示OFFのため非表示");
                } else if (resolved.visible && !frameExists) {
                    note = u8c(u8"セル内にその作画Fがない");
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(cellLabel.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(cellState.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(sourceLabel);
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(displayLabel.c_str());
                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(note.c_str());
            }
            ImGui::EndTable();
        }

        ImGui::Text(
            u8c(u8"表示セルN=%d / 候補セル=%d"),
            resolvedVisibleCount,
            static_cast<int>(timesheetSummaryCells.size()));
        ImGui::TextDisabled(u8c(u8"この一覧は確認用です。編集対象の作画Fはここでは変更しません。"));
        ImGui::Separator();

        // Timesheet Rebuild Step 7.5:
        // 選択T/セルについて、前後の原画とその間の中割を検出する。
        // ここではまだ中割を自動配置せず、次のStep 7.6のための確認UIに留める。
        if (!timesheetPanelData.cells.empty()) {
            const int safeSelectedCellColumn = std::clamp(
                timesheetPanelState_.selectedCellColumn,
                0,
                static_cast<int>(timesheetPanelData.cells.size()) - 1);
            const ::perapera::ui::TimesheetPanelCellColumn& selectedColumn =
                timesheetPanelData.cells[static_cast<std::size_t>(safeSelectedCellColumn)];
            int selectedProjectCellIndex = -1;
            Cell* selectedCell = projectCellById(project_, selectedColumn.cellId, &selectedProjectCellIndex);
            const ResolvedTimesheetInbetweenInterval interval = resolveTimesheetInbetweenInterval(
                workingTimesheet_,
                selectedColumn.cellId,
                resolvedTimelineFrame);

            ImGui::SeparatorText(u8c(u8"原画間検出 / 中割 / ライトテーブル"));
            ImGui::Text(
                u8c(u8"対象: %s / T%d"),
                selectedColumn.displayName.empty() ? selectedColumn.cellId.c_str() : selectedColumn.displayName.c_str(),
                interval.selectedTimelineFrame);
            ImGui::TextDisabled(u8c(u8"原画を先に置き、その間に中割を作るための検出結果です。"));

            if (!interval.previousKey.found && !interval.nextKey.found) {
                ImGui::TextUnformatted(u8c(u8"このセルにはまだ原画がありません。まずTに原画を置いてください。"));
            } else {
                const std::string previousKeyLabel = interval.previousKey.found
                    ? std::string(u8c(u8"T")) + std::to_string(interval.previousKey.timelineFrame) +
                        std::string(u8c(u8" / 作画F")) + std::to_string(interval.previousKey.drawingFrameNumber)
                    : std::string(u8c(u8"なし"));
                const std::string nextKeyLabel = interval.nextKey.found
                    ? std::string(u8c(u8"T")) + std::to_string(interval.nextKey.timelineFrame) +
                        std::string(u8c(u8" / 作画F")) + std::to_string(interval.nextKey.drawingFrameNumber)
                    : std::string(u8c(u8"なし"));

                if (ImGui::BeginTable(
                        "TimesheetInbetweenIntervalTable_v1",
                        2,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
                    ImGui::TableSetupColumn(u8c(u8"項目"));
                    ImGui::TableSetupColumn(u8c(u8"内容"));
                    ImGui::TableHeadersRow();

                    auto tableRow = [](const char* label, const std::string& value) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(label);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(value.c_str());
                    };

                    tableRow(u8c(u8"前原画"), previousKeyLabel);
                    tableRow(u8c(u8"次原画"), nextKeyLabel);
                    tableRow(
                        u8c(u8"選択T"),
                        interval.selectedIsKey
                            ? std::string(u8c(u8"原画"))
                            : (interval.selectedIsInbetween
                                ? std::string(u8c(u8"中割"))
                                : (interval.selectedInsideInterval
                                    ? std::string(u8c(u8"原画間の空きT"))
                                    : std::string(u8c(u8"原画間の外")))));
                    tableRow(
                        u8c(u8"原画間位置"),
                        interval.hasClosedKeyInterval
                            ? std::to_string(interval.selectedPositionNumerator) + "/" + std::to_string(interval.selectedPositionDenominator)
                            : std::string(u8c(u8"未確定")));
                    tableRow(
                        u8c(u8"中割"),
                        std::to_string(interval.usedInbetweenCount) +
                            std::string(u8c(u8"枚 / 配置候補T")) +
                            std::to_string(interval.availableTimelineSlotCount));
                    ImGui::EndTable();
                }

                if (interval.hasClosedKeyInterval) {
                    if (interval.inbetweens.empty()) {
                        ImGui::TextUnformatted(u8c(u8"この原画間にはまだ中割がありません。"));
                    } else {
                        ImGui::TextUnformatted(u8c(u8"この原画間の中割:"));
                        for (const TimesheetInbetweenDrawingInfo& inbetween : interval.inbetweens) {
                            ImGui::BulletText(
                                u8c(u8"%d/%d: T%d / 作画F%d / 役割: 中割"),
                                inbetween.inbetweenIndex,
                                inbetween.inbetweenCount,
                                inbetween.timelineFrame,
                                inbetween.drawingFrameNumber);
                        }
                    }

                    if (ImGui::SmallButton(u8c(u8"この原画間を再生T範囲へ##SetRangeFromKeyInterval"))) {
                        setTimesheetPlaybackRangeFromOneBased(timesheetPanelData,
                            interval.previousKey.timelineFrame,
                            interval.nextKey.timelineFrame,
                            "timesheet range from key interval");
                        isPlayingTimesheet_ = false;
                        isPlayingTimesheetRange_ = false;
                        timesheetPlaybackAccumulator_ = 0.0f;
                    }

                    ImGui::SeparatorText(u8c(u8"中割ライトテーブル / 再生順オニオン"));
                    ImGui::TextDisabled(u8c(u8"中割作画では、前原画と次原画を候補としてすぐライトテーブルに出せます。"));
                    ImGui::TextDisabled(u8c(u8"オニオンは作画F番号順ではなく、タイムシートT順の再生順を優先します。"));

                    std::string playbackOnionLabel = u8c(u8"再生順オニオン: 未確定");
                    if (selectedCell != nullptr) {
                        const std::vector<int> selectedPlaybackOrder =
                            buildTimesheetPlaybackOrderFrameIndicesForCell(workingTimesheet_, *selectedCell);
                        const int playbackPrevious = adjacentPlaybackOrderFrameIndex(selectedPlaybackOrder, activeFrameIndex_, -1);
                        const int playbackNext = adjacentPlaybackOrderFrameIndex(selectedPlaybackOrder, activeFrameIndex_, 1);
                        playbackOnionLabel = std::string(u8c(u8"再生順オニオン: 前 ")) +
                            (playbackPrevious >= 0 ? (std::string(u8c(u8"作画F")) + std::to_string(playbackPrevious + 1)) : std::string(u8c(u8"なし"))) +
                            std::string(u8c(u8" / 次 ")) +
                            (playbackNext >= 0 ? (std::string(u8c(u8"作画F")) + std::to_string(playbackNext + 1)) : std::string(u8c(u8"なし")));
                    }
                    ImGui::TextUnformatted(playbackOnionLabel.c_str());

                    std::vector<int> keyLightTableFrameIndices;
                    if (interval.previousKey.found) {
                        const int previousKeyFrameIndex = interval.previousKey.drawingFrameNumber - 1;
                        if (selectedCell != nullptr &&
                            previousKeyFrameIndex >= 0 &&
                            previousKeyFrameIndex < static_cast<int>(selectedCell->frames.size()) &&
                            previousKeyFrameIndex != activeFrameIndex_) {
                            keyLightTableFrameIndices.push_back(previousKeyFrameIndex);
                        }
                    }
                    if (interval.nextKey.found) {
                        const int nextKeyFrameIndex = interval.nextKey.drawingFrameNumber - 1;
                        if (selectedCell != nullptr &&
                            nextKeyFrameIndex >= 0 &&
                            nextKeyFrameIndex < static_cast<int>(selectedCell->frames.size()) &&
                            nextKeyFrameIndex != activeFrameIndex_ &&
                            std::find(keyLightTableFrameIndices.begin(),
                                      keyLightTableFrameIndices.end(),
                                      nextKeyFrameIndex) == keyLightTableFrameIndices.end()) {
                            keyLightTableFrameIndices.push_back(nextKeyFrameIndex);
                        }
                    }

                    ImGui::Text(
                        u8c(u8"候補: 前原画 作画F%d / 次原画 作画F%d"),
                        interval.previousKey.drawingFrameNumber,
                        interval.nextKey.drawingFrameNumber);
                    if (keyLightTableFrameIndices.empty()) {
                        ImGui::BeginDisabled();
                    }
                    if (ImGui::SmallButton(u8c(u8"前後原画をライトテーブルに反映##ApplyKeyLightTable"))) {
                        activeCellIndex_ = selectedProjectCellIndex;
                        lightTableFrameIndices_ = keyLightTableFrameIndices;
                        lightTableEnabled_ = !lightTableFrameIndices_.empty();
                        lightTableOpacity_ = std::max(lightTableOpacity_, 0.35f);
                        canvasRenderer_.markAllDirty();
                        lastMessage_ = "inbetween light table: key F" +
                            std::to_string(interval.previousKey.drawingFrameNumber) +
                            " -> F" +
                            std::to_string(interval.nextKey.drawingFrameNumber);
                    }
                    if (keyLightTableFrameIndices.empty()) {
                        ImGui::EndDisabled();
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton(u8c(u8"ライトテーブルを消す##ClearKeyLightTable"))) {
                        lightTableFrameIndices_.clear();
                        lightTableEnabled_ = false;
                        canvasRenderer_.markAllDirty();
                        lastMessage_ = "light table cleared";
                    }

                    ImGui::SeparatorText(u8c(u8"中割作成"));
                    ImGui::TextDisabled(u8c(u8"中割は新しい作画Fとして追加し、原画Fは上書きしません。"));
                    ImGui::TextDisabled(u8c(u8"枚数指定ではなく、1コマ打ち/2コマ打ち/3コマ打ち/nコマ打ちで配置します。"));

                    static int inbetweenKomaStep = 2;
                    ImGui::SetNextItemWidth(96.0f);
                    if (ImGui::InputInt(u8c(u8"nコマ打ち##InbetweenKomaStep"), &inbetweenKomaStep)) {
                        inbetweenKomaStep = std::clamp(inbetweenKomaStep, 1, 24);
                    }
                    inbetweenKomaStep = std::clamp(inbetweenKomaStep, 1, 24);
                    ImGui::SameLine();
                    ImGui::TextDisabled(u8c(u8"例: T1→T7の2コマ打ちはT3/T5"));

                    auto addInbetweensByKomaStep = [&](int komaStep) {
                        komaStep = std::clamp(komaStep, 1, 24);

                        int selectedProjectCellIndex = -1;
                        Cell* targetCell = projectCellById(project_, selectedColumn.cellId, &selectedProjectCellIndex);
                        if (targetCell == nullptr) {
                            lastMessage_ = "inbetween add failed: selected cell not found";
                            return;
                        }

                        const TimesheetInbetweenPlacementPlan plan = planTimesheetInbetweenPlacementsForKomaStep(
                            workingTimesheet_,
                            selectedColumn.cellId,
                            interval.previousKey.timelineFrame,
                            interval.nextKey.timelineFrame,
                            komaStep);
                        if (!plan.ok) {
                            lastMessage_ = "inbetween " + std::to_string(komaStep) + "-koma add failed: " + plan.message;
                            return;
                        }

                        if (targetCell->frames.empty()) {
                            targetCell->frames.push_back(Frame::createDefault(1));
                        }

                        std::vector<int> createdDrawingFrames;
                        createdDrawingFrames.reserve(plan.timelineFrames.size());
                        for (int timelineFrame : plan.timelineFrames) {
                            const int newDrawingFrameNumber = static_cast<int>(targetCell->frames.size()) + 1;
                            targetCell->frames.push_back(Frame::createDefault(newDrawingFrameNumber));
                            createdDrawingFrames.push_back(newDrawingFrameNumber);

                            ::perapera::ui::TimesheetPanelEditableEntry entry;
                            entry.timelineFrame = std::max(0, timelineFrame - 1);
                            entry.cellId = selectedColumn.cellId;
                            entry.kind = ::perapera::ui::TimesheetPanelEntryKind::Inbetween;
                            entry.drawingFrameNumber = newDrawingFrameNumber;
                            timesheetPanelState_.entries.push_back(std::move(entry));
                        }

                        const bool normalizedAfterAdd =
                            ::perapera::ui::normalizeTimesheetPanelStateForViewModel(timesheetPanelData, timesheetPanelState_);
                        (void)normalizedAfterAdd;
                        workingTimesheet_ = ::perapera::ui::buildTimesheetFromPanelState(timesheetPanelData, timesheetPanelState_);
                        workingTimesheetDirty_ = true;

                        activeCellIndex_ = selectedProjectCellIndex;
                        activeFrameIndex_ = std::max(0, createdDrawingFrames.front() - 1);
                        activeLayerIndex_ = 0;
                        timesheetPanelState_.selectedTimelineFrame = std::max(0, plan.timelineFrames.front() - 1);
                        timesheetPanelState_.selectedCellColumn = safeSelectedCellColumn;
                        timesheetPanelState_.editDrawingFrameNumber = createdDrawingFrames.front();
                        isPlayingTimesheet_ = false;
                        isPlayingTimesheetRange_ = false;
                        isPlayingFrames_ = false;
                        timesheetPlaybackAccumulator_ = 0.0f;
                        playbackAccumulator_ = 0.0f;
                        canvasRenderer_.clearLayerCaches();
                        canvasRenderer_.markAllDirty();

                        std::ostringstream message;
                        message << "inbetween " << komaStep << "-koma added: ";
                        for (std::size_t i = 0; i < plan.timelineFrames.size(); ++i) {
                            if (i != 0U) {
                                message << ", ";
                            }
                            message << "T" << plan.timelineFrames[i]
                                    << "=作画F" << createdDrawingFrames[i];
                        }
                        message << " / key F" << interval.previousKey.drawingFrameNumber
                                << "->F" << interval.nextKey.drawingFrameNumber;
                        lastMessage_ = message.str();
                    };

                    if (ImGui::SmallButton(u8c(u8"1コマ打ち##AddInbetweenKoma1"))) {
                        addInbetweensByKomaStep(1);
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton(u8c(u8"2コマ打ち##AddInbetweenKoma2"))) {
                        addInbetweensByKomaStep(2);
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton(u8c(u8"3コマ打ち##AddInbetweenKoma3"))) {
                        addInbetweensByKomaStep(3);
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton(u8c(u8"指定nコマで追加##AddInbetweenKomaN"))) {
                        addInbetweensByKomaStep(inbetweenKomaStep);
                    }
                    ImGui::TextDisabled(
                        u8c(u8"追加後は最初の中割作画Fへ編集対象を切り替えます。既に埋まっているTは壊さず飛ばします。"));
                } else {
                    ImGui::TextDisabled(u8c(u8"前後原画が両方あると、中割作成候補として扱えます。"));
                }
            }
            ImGui::TextDisabled(u8c(u8"絵コンテ・レイアウト・背景参照はセル列に混ぜず、Scene Plateとして別管理します。"));
        }
        ImGui::End();
    }
    const bool isColoringMode = currentMode_ == AppMode::Coloring;
    if (isColoringMode) {
        selectPaintLayerForColoring(true);
    }
    clampSelection();
    handleFrameShortcuts();
    updateFramePlayback();
    // Timesheet Rebuild Step 7.3:
    // キャンバス上ホイール時に親ワークスペースが縦スクロールしないようにする。
  ::perapera::app_drawing::drawDrawingWorkspaceLayout(
      {
          [this]() { drawLeftSidebar(); },
          [this](float rightWidth) { drawCanvasArea(rightWidth); },
          [this]() { drawRightSidebar(); },
          [this]() { drawTimelineArea(); },
      });
}
} // namespace perapera
