// このファイルの役割:
// 保存・読み込み・書き出しパネルの実装。
// MP4失敗時に原因を追えるよう、FFmpegログの場所をUIに表示する。

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
    ImGui::PushID("ExportPanel_v2");
    ImGui::TextUnformatted(u8c(u8"保存 / 書き出し"));
    ImGui::InputText(u8c(u8"プロジェクト##Export_ProjectFolder"), state.projectFolder, sizeof(state.projectFolder));

    ExportPanelAction action = ExportPanelAction::None;
    if (ImGui::Button(u8c(u8"保存##Export_SaveProject"))) {
        action = ExportPanelAction::SaveProject;
    }
    ImGui::SameLine();
    if (ImGui::Button(u8c(u8"読み込み##Export_LoadProject"))) {
        action = ExportPanelAction::LoadProject;
    }

    ImGui::Separator();
    ImGui::InputText(u8c(u8"PNG出力先##Export_PngFolder"), state.exportFolder, sizeof(state.exportFolder));
    if (ImGui::Button(u8c(u8"現在フレームPNG##Export_ActivePng"))) {
        action = ExportPanelAction::ExportActivePng;
    }
    ImGui::SameLine();
    if (ImGui::Button(u8c(u8"PNG連番##Export_PngSequence"))) {
        action = ExportPanelAction::ExportPngSequence;
    }

    ImGui::Separator();
    ImGui::InputText("FFmpeg##Export_FfmpegPath", state.ffmpegPath, sizeof(state.ffmpegPath));
    ImGui::TextDisabled("ffmpeg  または  C:/ffmpeg/bin/ffmpeg.exe");
    ImGui::InputText("MP4##Export_Mp4Path", state.mp4Path, sizeof(state.mp4Path));
    if (ImGui::Button(u8c(u8"MP4書き出し##Export_Mp4"))) {
        action = ExportPanelAction::ExportMp4;
    }
    ImGui::TextDisabled("MP4失敗時ログ: exports/mp4/ffmpeg_last_run.log");

    ImGui::Separator();
    ImGui::TextWrapped("%s", lastMessage == nullptr ? "" : lastMessage);
    ImGui::PopID();
    return action;
}

} // namespace perapera::ui
