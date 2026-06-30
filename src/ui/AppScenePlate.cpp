// Scene Plate manager UI split from AppDrawingMode.
#include "ui/App.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <utility>

#include <imgui.h>

#include "io/CutIO.h"
#include "ui/AppProjectIOSupport.h"
#include "ui/AppScenePlateSupport.h"

namespace perapera {

void App::drawScenePlateManager()
{
    using namespace scene_plate_ui;
    // Timesheet Rebuild Step 7.15:
    // Scene Plate をセル列やタイムシート列へ混ぜず、一覧UIで操作し、
    // 画像パスが有効な場合は実画像をキャンバス背景として表示する。
    // PNG/MP4出力反映は後続Stepへ分ける。
    {
        const int currentT = std::max(1, timesheetPanelState_.selectedTimelineFrame + 1);
        const std::filesystem::path projectFolder = appio::absolutePath(exportState_.projectFolder);
        normalizeScenePlateStack(workingScenePlates_);
        clampScenePlateSelection(workingScenePlates_, selectedScenePlateIndex_);

        ImGui::Begin(
            u8c(u8"Scene Plate管理##ScenePlateManager"),
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::TextUnformatted(u8c(u8"Step 7.15: Scene Plate 実画像キャンバス表示"));
        ImGui::TextDisabled(u8c(u8"絵コンテ/レイアウト/仮背景/完成背景をセル列とは別に管理します。"));
        ImGui::TextDisabled(u8c(u8"画像パスが有効なら実画像をキャンバスに表示します。失敗時はダミー矩形に戻します。"));
        if (ImGui::Checkbox(u8c(u8"キャンバス仮表示##ScenePlateCanvasPreviewEnabled"), &scenePlateCanvasPreviewEnabled_)) {
            lastMessage_ = scenePlateCanvasPreviewEnabled_
                ? "scene plate canvas preview enabled"
                : "scene plate canvas preview disabled";
        }
        ImGui::SameLine();
        ImGui::TextDisabled(u8c(u8"visible / 不透明度 / zOrder / T範囲 / 位置 / 拡大率 / 回転を反映"));

        if (ImGui::SmallButton(u8c(u8"絵コンテ追加##AddScenePlateStoryboard"))) {
            workingScenePlates_.plates.push_back(makeDefaultScenePlate(workingScenePlates_, ScenePlateKind::Storyboard));
            selectedScenePlateIndex_ = static_cast<int>(workingScenePlates_.plates.size()) - 1;
            workingScenePlatesDirty_ = true;
            lastMessage_ = "scene plate added: storyboard";
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(u8c(u8"レイアウト追加##AddScenePlateLayout"))) {
            workingScenePlates_.plates.push_back(makeDefaultScenePlate(workingScenePlates_, ScenePlateKind::Layout));
            selectedScenePlateIndex_ = static_cast<int>(workingScenePlates_.plates.size()) - 1;
            workingScenePlatesDirty_ = true;
            lastMessage_ = "scene plate added: layout";
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(u8c(u8"参照画像追加##AddScenePlateReference"))) {
            workingScenePlates_.plates.push_back(makeDefaultScenePlate(workingScenePlates_, ScenePlateKind::ReferenceImage));
            selectedScenePlateIndex_ = static_cast<int>(workingScenePlates_.plates.size()) - 1;
            workingScenePlatesDirty_ = true;
            lastMessage_ = "scene plate added: reference image";
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(u8c(u8"仮背景追加##AddScenePlateTemporaryBackground"))) {
            workingScenePlates_.plates.push_back(makeDefaultScenePlate(workingScenePlates_, ScenePlateKind::TemporaryBackground));
            selectedScenePlateIndex_ = static_cast<int>(workingScenePlates_.plates.size()) - 1;
            workingScenePlatesDirty_ = true;
            lastMessage_ = "scene plate added: temporary background";
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(u8c(u8"完成背景追加##AddScenePlateFinalBackground"))) {
            workingScenePlates_.plates.push_back(makeDefaultScenePlate(workingScenePlates_, ScenePlateKind::FinalBackground));
            selectedScenePlateIndex_ = static_cast<int>(workingScenePlates_.plates.size()) - 1;
            workingScenePlatesDirty_ = true;
            lastMessage_ = "scene plate added: final background";
        }

        int visibleAtCurrentT = 0;
        int previewAtCurrentT = 0;
        int renderAtCurrentT = 0;
        for (const ScenePlate& plate : workingScenePlates_.plates) {
            if (scenePlateVisibleAtTimelineFrame(plate, currentT)) {
                ++visibleAtCurrentT;
            }
            if (scenePlateParticipatesInPreview(plate, currentT)) {
                ++previewAtCurrentT;
            }
            if (scenePlateParticipatesInRenderOutput(plate, currentT)) {
                ++renderAtCurrentT;
            }
        }

        ImGui::Text(
            u8c(u8"T%d: 表示候補=%d / プレビュー候補=%d / 出力候補=%d / total=%d%s"),
            currentT,
            visibleAtCurrentT,
            previewAtCurrentT,
            renderAtCurrentT,
            static_cast<int>(workingScenePlates_.plates.size()),
            workingScenePlatesDirty_ ? u8c(u8" / 未保存の可能性") : u8c(u8""));

        if (ImGui::BeginTable(
                "ScenePlateListTable_v1",
                8,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn(u8c(u8"選択"));
            ImGui::TableSetupColumn(u8c(u8"表示"));
            ImGui::TableSetupColumn(u8c(u8"名前"));
            ImGui::TableSetupColumn(u8c(u8"種類"));
            ImGui::TableSetupColumn(u8c(u8"用途"));
            ImGui::TableSetupColumn(u8c(u8"不透明度"));
            ImGui::TableSetupColumn(u8c(u8"T範囲"));
            ImGui::TableSetupColumn(u8c(u8"順"));
            ImGui::TableHeadersRow();

            for (int plateIndex = 0; plateIndex < static_cast<int>(workingScenePlates_.plates.size()); ++plateIndex) {
                ScenePlate& plate = workingScenePlates_.plates[static_cast<std::size_t>(plateIndex)];
                if (plate.id.empty()) {
                    ImGui::PushID(plateIndex);
                } else {
                    ImGui::PushID(plate.id.c_str());
                }
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                const bool selected = plateIndex == selectedScenePlateIndex_;
                if (ImGui::Selectable("##SelectScenePlate", selected, ImGuiSelectableFlags_SpanAllColumns)) {
                    selectedScenePlateIndex_ = plateIndex;
                }

                ImGui::TableSetColumnIndex(1);
                if (ImGui::Checkbox("##ScenePlateVisible", &plate.visible)) {
                    workingScenePlatesDirty_ = true;
                    lastMessage_ = "scene plate visible changed";
                }

                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(plate.displayName.empty() ? plate.id.c_str() : plate.displayName.c_str());

                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(scenePlateKindJapaneseLabel(plate.kind));

                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(scenePlateOutputModeJapaneseLabel(plate.outputMode));

                ImGui::TableSetColumnIndex(5);
                ImGui::Text(u8c(u8"%.2f"), plate.opacity);

                ImGui::TableSetColumnIndex(6);
                if (plate.startTimelineFrame <= 0 && plate.endTimelineFrame <= 0) {
                    ImGui::TextUnformatted(u8c(u8"全T"));
                } else {
                    ImGui::Text(
                        u8c(u8"T%d-T%d"),
                        plate.startTimelineFrame <= 0 ? 1 : plate.startTimelineFrame,
                        plate.endTimelineFrame <= 0 ? std::max(1, project_.timeline.totalFrames) : plate.endTimelineFrame);
                }

                ImGui::TableSetColumnIndex(7);
                ImGui::Text("%d", plate.zOrder);
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        clampScenePlateSelection(workingScenePlates_, selectedScenePlateIndex_);
        ImGui::SeparatorText(u8c(u8"選択Scene Plate"));
        if (workingScenePlates_.plates.empty()) {
            ImGui::TextDisabled(u8c(u8"まだScene Plateがありません。上の追加ボタンで作成してください。"));
        } else {
            ScenePlate& selectedPlate = workingScenePlates_.plates[static_cast<std::size_t>(selectedScenePlateIndex_)];
            bool changed = false;

            ImGui::Text(u8c(u8"ID: %s"), selectedPlate.id.c_str());
            changed = inputScenePlateText(u8c(u8"表示名##ScenePlateDisplayName"), selectedPlate.displayName) || changed;

            int kindIndex = scenePlateKindIndex(selectedPlate.kind);
            const char* kindItems[] = {
                u8c(u8"絵コンテ"),
                u8c(u8"レイアウト/背景原図"),
                u8c(u8"参照画像"),
                u8c(u8"仮背景"),
                u8c(u8"完成背景"),
            };
            if (ImGui::Combo(u8c(u8"種類##ScenePlateKind"), &kindIndex, kindItems, IM_ARRAYSIZE(kindItems))) {
                selectedPlate.kind = scenePlateKindFromIndex(kindIndex);
                changed = true;
            }

            int outputModeIndex = scenePlateOutputModeIndex(selectedPlate.outputMode);
            const char* outputModeItems[] = {
                u8c(u8"参照のみ"),
                u8c(u8"プレビューのみ"),
                u8c(u8"出力対象"),
            };
            if (ImGui::Combo(u8c(u8"用途##ScenePlateOutputMode"), &outputModeIndex, outputModeItems, IM_ARRAYSIZE(outputModeItems))) {
                selectedPlate.outputMode = scenePlateOutputModeFromIndex(outputModeIndex);
                changed = true;
            }
            ImGui::TextDisabled(u8c(u8"表示ON/OFFと用途は別です。参照のみは正式出力へ含めません。"));

            if (ImGui::Checkbox(u8c(u8"表示ON##ScenePlateVisibleDetail"), &selectedPlate.visible)) {
                changed = true;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox(u8c(u8"ロック##ScenePlateLockedDetail"), &selectedPlate.locked)) {
                changed = true;
            }
            ImGui::SameLine();
            if (ImGui::Button(u8c(u8"正規化##NormalizeScenePlateStack"))) {
                normalizeScenePlateStack(workingScenePlates_);
                clampScenePlateSelection(workingScenePlates_, selectedScenePlateIndex_);
                changed = true;
            }

            if (ImGui::SliderFloat(u8c(u8"不透明度##ScenePlateOpacity"), &selectedPlate.opacity, 0.0f, 1.0f, "%.2f")) {
                changed = true;
            }
            if (ImGui::InputInt(u8c(u8"表示順 zOrder##ScenePlateZOrder"), &selectedPlate.zOrder)) {
                changed = true;
            }

            int startT = selectedPlate.startTimelineFrame;
            int endT = selectedPlate.endTimelineFrame;
            ImGui::SetNextItemWidth(110.0f);
            if (ImGui::InputInt(u8c(u8"開始T 0=制限なし##ScenePlateStartT"), &startT)) {
                selectedPlate.startTimelineFrame = startT;
                changed = true;
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(110.0f);
            if (ImGui::InputInt(u8c(u8"終了T 0=制限なし##ScenePlateEndT"), &endT)) {
                selectedPlate.endTimelineFrame = endT;
                changed = true;
            }

            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::InputFloat2(u8c(u8"位置XY##ScenePlatePosition"), &selectedPlate.transform.x, "%.1f")) {
                changed = true;
            }
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::InputFloat2(u8c(u8"拡大率XY##ScenePlateScale"), &selectedPlate.transform.scaleX, "%.3f")) {
                changed = true;
            }
            ImGui::SetNextItemWidth(160.0f);
            if (ImGui::InputFloat(u8c(u8"回転deg##ScenePlateRotation"), &selectedPlate.transform.rotationDegrees, 1.0f, 10.0f, "%.1f")) {
                changed = true;
            }

            ImGui::SeparatorText(u8c(u8"画像パス"));
            if (inputScenePlateText(u8c(u8"画像パス##ScenePlateImagePath"), selectedPlate.imagePath)) {
                const std::string cleanPathText = trimScenePlateImagePathText(selectedPlate.imagePath);
                if (selectedPlate.imagePath != cleanPathText) {
                    selectedPlate.imagePath = cleanPathText;
                }
                scenePlateImageCache_.clear();
                changed = true;
            }
            const std::string imageStatus = scenePlateImageStatusLabel(selectedPlate, projectFolder);
            const std::string imageDetail = scenePlateImageDetailLabel(selectedPlate, projectFolder);
            const ScenePlateImagePathInfo imagePathInfo =
                resolveScenePlateImagePathInfo(selectedPlate.imagePath, projectFolder);
            const std::string& cleanImagePathText = imagePathInfo.normalizedText;
            ImGui::TextWrapped(u8c(u8"状態: %s"), imageStatus.c_str());
            ImGui::TextWrapped(
                u8c(u8"入力パス: %s"),
                selectedPlate.imagePath.empty() ? u8c(u8"未指定") : selectedPlate.imagePath.c_str());
            ImGui::TextWrapped(u8c(u8"正規化後: %s"), cleanImagePathText.empty() ? u8c(u8"未指定") : cleanImagePathText.c_str());
            ImGui::TextWrapped(u8c(u8"絶対パス判定: %s"), imagePathInfo.isAbsolute ? "YES" : "NO");
            ImGui::TextWrapped(u8c(u8"projectFolder: %s"), projectFolder.string().c_str());
            ImGui::TextWrapped(u8c(u8"%s"), imageDetail.c_str());
            const std::filesystem::path selectedResolvedImagePath = imagePathInfo.resolvedPath;
            ImGui::TextWrapped(
                u8c(u8"キャッシュ読込パス: %s"),
                selectedResolvedImagePath.empty() ? u8c(u8"未指定") : selectedResolvedImagePath.string().c_str());
#if defined(_WIN32)
            if (!cleanImagePathText.empty()) {
                const bool directAnsiExists = scenePlateWin32RegularFileExistsA(cleanImagePathText);
                ImGui::TextDisabled(
                    u8c(u8"Win32確認: 入力=%s / 解決=%s"),
                    (directAnsiExists || imagePathInfo.inputExists) ? "OK" : "NG",
                    imagePathInfo.resolvedExists ? "OK" : "NG");
            }
#endif
            if (!selectedPlate.imagePath.empty() && scenePlateImageExtensionSupported(selectedResolvedImagePath)) {
                const ScenePlateImageCache::TextureResult& textureResult = scenePlateImageCache_.textureFor(renderer_, selectedResolvedImagePath);
                ImGui::TextWrapped(u8c(u8"読込: %s"), textureResult.status.empty() ? u8c(u8"未実行") : textureResult.status.c_str());
                if (textureResult.ok) {
                    ImGui::Text(u8c(u8"画像サイズ: %d x %d"), textureResult.width, textureResult.height);
                }
            }
            ImGui::TextDisabled(u8c(u8"対応候補: .png / .jpg / .jpeg / .bmp。WebPはWindows環境のWIC対応に依存します。"));
            if (ImGui::SmallButton(u8c(u8"画像パス確認##ScenePlateCheckImagePath"))) {
                scenePlateImageCache_.clear();
                lastMessage_ = imageStatus;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton(u8c(u8"プロジェクト相対パス化##ScenePlateRelativeImagePath"))) {
                if (scenePlateMakeImagePathRelativeToProject(selectedPlate, projectFolder)) {
                    scenePlateImageCache_.clear();
                    changed = true;
                    lastMessage_ = "scene plate image path made relative";
                } else {
                    lastMessage_ = "scene plate image path was not changed";
                }
            }
            ImGui::SameLine();
            if (ImGui::SmallButton(u8c(u8"画像パス消去##ScenePlateClearImagePath"))) {
                selectedPlate.imagePath.clear();
                scenePlateImageCache_.clear();
                changed = true;
                lastMessage_ = "scene plate image path cleared";
            }

            ImGui::SameLine();
            if (ImGui::SmallButton(u8c(u8"画像キャッシュ再読込##ScenePlateReloadImageCache"))) {
                scenePlateImageCache_.clear();
                lastMessage_ = "scene plate image cache cleared";
            }

            const bool canMoveUp = selectedScenePlateIndex_ > 0;
            const bool canMoveDown = selectedScenePlateIndex_ + 1 < static_cast<int>(workingScenePlates_.plates.size());
            if (!canMoveUp) {
                ImGui::BeginDisabled();
            }
            if (ImGui::SmallButton(u8c(u8"上へ##ScenePlateMoveUp"))) {
                std::swap(
                    workingScenePlates_.plates[static_cast<std::size_t>(selectedScenePlateIndex_)],
                    workingScenePlates_.plates[static_cast<std::size_t>(selectedScenePlateIndex_ - 1)]);
                --selectedScenePlateIndex_;
                assignScenePlateSequentialZOrder(workingScenePlates_);
                changed = true;
            }
            if (!canMoveUp) {
                ImGui::EndDisabled();
            }
            ImGui::SameLine();
            if (!canMoveDown) {
                ImGui::BeginDisabled();
            }
            if (ImGui::SmallButton(u8c(u8"下へ##ScenePlateMoveDown"))) {
                std::swap(
                    workingScenePlates_.plates[static_cast<std::size_t>(selectedScenePlateIndex_)],
                    workingScenePlates_.plates[static_cast<std::size_t>(selectedScenePlateIndex_ + 1)]);
                ++selectedScenePlateIndex_;
                assignScenePlateSequentialZOrder(workingScenePlates_);
                changed = true;
            }
            if (!canMoveDown) {
                ImGui::EndDisabled();
            }
            ImGui::SameLine();
            if (ImGui::SmallButton(u8c(u8"複製##ScenePlateDuplicate"))) {
                ScenePlate duplicate = selectedPlate;
                duplicate.id = nextScenePlateId(workingScenePlates_);
                duplicate.displayName += u8c(u8" copy");
                duplicate.zOrder = nextScenePlateZOrder(workingScenePlates_);
                workingScenePlates_.plates.push_back(std::move(duplicate));
                selectedScenePlateIndex_ = static_cast<int>(workingScenePlates_.plates.size()) - 1;
                changed = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton(u8c(u8"削除##ScenePlateDelete"))) {
                workingScenePlates_.plates.erase(workingScenePlates_.plates.begin() + selectedScenePlateIndex_);
                clampScenePlateSelection(workingScenePlates_, selectedScenePlateIndex_);
                changed = true;
            }

            if (changed) {
                normalizeScenePlateStack(workingScenePlates_);
                clampScenePlateSelection(workingScenePlates_, selectedScenePlateIndex_);
                workingScenePlatesDirty_ = true;
                canvasRenderer_.markAllDirty();
                lastMessage_ = "scene plate edited: count=" + std::to_string(workingScenePlates_.plates.size());
            }
        }

        ImGui::SeparatorText(u8c(u8"cut.json 試験保存 / 読み込み"));
        ImGui::TextWrapped(u8c(u8"保存先: %s"), CutIO::cutJsonPathForCutFolder(projectFolder).string().c_str());
        if (ImGui::SmallButton(u8c(u8"Scene Plateをcut.jsonへ保存##SaveScenePlatesCutJson"))) {
            Cut cut;
            cut.id = project_.name.empty() ? "cut_001" : project_.name;
            cut.name = project_.name;
            cut.frameRate = std::max(1, project_.output.fps);
            cut.totalFrames = std::max(1, project_.timeline.totalFrames);
            cut.timesheet = workingTimesheet_;
            cut.timesheet.totalFrames = cut.totalFrames;
            cut.scenePlates = workingScenePlates_;
            for (const Cell& cell : project_.cells) {
                cut.cellZOrderKeys.push_back(cell.id);
            }
            std::string error;
            if (CutIO::saveCut(cut, projectFolder, &error)) {
                workingScenePlatesDirty_ = false;
                workingTimesheetDirty_ = false;
                lastMessage_ = "scene plates saved to cut.json: " + projectFolder.string();
            } else {
                lastMessage_ = "scene plate cut save failed: " + error;
            }
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(u8c(u8"cut.jsonからScene Plate読込##LoadScenePlatesCutJson"))) {
            Cut loadedCut;
            std::string error;
            if (CutIO::loadCut(projectFolder, loadedCut, &error)) {
                workingScenePlates_ = std::move(loadedCut.scenePlates);
                normalizeScenePlateStack(workingScenePlates_);
                clampScenePlateSelection(workingScenePlates_, selectedScenePlateIndex_);
                workingScenePlatesDirty_ = false;
                lastMessage_ = "scene plates loaded from cut.json: count=" + std::to_string(workingScenePlates_.plates.size());
            } else {
                lastMessage_ = "scene plate cut load failed: " + error;
            }
        }
        ImGui::TextDisabled(u8c(u8"通常Project保存とはまだ統合していません。Scene Plateを残すにはこの試験保存を使います。"));
        ImGui::End();
    }
}

} // namespace perapera
