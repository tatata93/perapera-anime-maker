// このファイルの役割:
// 保存・読み込み・書き出しパネルの実装。
// MP4失敗時に原因を追えるよう、MP4ボタン直下に実行結果を常時表示する。

#include "ui/panels/ExportPanel.h"

#include <imgui.h>

namespace perapera::ui {
namespace {

const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

} // namespace

ExportPanelAction drawExportPanel(ExportPanelState& state, const char* lastMessage)
{
    ImGui::PushID("ExportPanel_v11_stability");
    ImGui::TextUnformatted(u8c(u8"保存 / 書き出し"));

    ImGui::InputText(u8c(u8"プロジェクト##Export_ProjectFolder_v11"), state.projectFolder, sizeof(state.projectFolder));

    ExportPanelAction action = ExportPanelAction::None;
    if (ImGui::Button(u8c(u8"保存##Export_SaveProject_v11"))) {
        action = ExportPanelAction::SaveProject;
    }
    ImGui::SameLine();
    if (ImGui::Button(u8c(u8"読み込み##Export_LoadProject_v11"))) {
        action = ExportPanelAction::LoadProject;
    }
    if (ImGui::Button(u8c(u8"保存→読込テスト##Export_VerifyRoundTrip_v11"), ImVec2(-1.0f, 0.0f))) {
        action = ExportPanelAction::VerifyProjectRoundTrip;
    }
    ImGui::TextDisabled(u8c(u8"保存後に読み直し、フレーム/線数が保たれるか確認します。"));

    ImGui::Separator();
    ImGui::InputText(u8c(u8"PNG出力先##Export_PngFolder_v11"), state.exportFolder, sizeof(state.exportFolder));
    ImGui::TextDisabled("default: exports/png");
    if (ImGui::Button(u8c(u8"現在フレームPNG##Export_ActivePng_v11"))) {
        action = ExportPanelAction::ExportActivePng;
    }
    ImGui::SameLine();
    if (ImGui::Button(u8c(u8"PNG連番##Export_PngSequence_v11"))) {
        action = ExportPanelAction::ExportPngSequence;
    }

    ImGui::Separator();
    ImGui::InputText("FFmpeg##Export_FfmpegPath_v11", state.ffmpegPath, sizeof(state.ffmpegPath));
    ImGui::TextDisabled("default: ffmpeg / full path is also OK");
    ImGui::InputText("MP4##Export_Mp4Path_v11", state.mp4Path, sizeof(state.mp4Path));
    if (ImGui::Button(u8c(u8"MP4書き出し##Export_Mp4_v11"))) {
        action = ExportPanelAction::ExportMp4;
    }
    ImGui::TextDisabled("log: exports/mp4/ffmpeg_last_run.log / v11");

    ImGui::Spacing();
    ImGui::TextUnformatted(u8c(u8"実行結果"));
    ImGui::BeginChild("Export_LastMessage_v11", ImVec2(0.0f, 110.0f), true,
        ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);
    const char* message = lastMessage == nullptr ? "" : lastMessage;
    ImGui::TextWrapped("%s", message);
    ImGui::EndChild();

    ImGui::PopID();
    return action;
}

} // namespace perapera::ui
