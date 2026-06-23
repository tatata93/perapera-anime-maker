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
    ImGui::PushID("ExportPanel_v4");
    ImGui::PushID(static_cast<const void*>(&state));
    ImGui::TextUnformatted(u8c(u8"保存 / 書き出し"));

    ImGui::PushID("ProjectFolderInput_v4");
    ImGui::InputText(u8c(u8"プロジェクト"), state.projectFolder, sizeof(state.projectFolder));
    ImGui::PopID();

    ExportPanelAction action = ExportPanelAction::None;
    ImGui::PushID("SaveProjectButton_v4");
    if (ImGui::Button(u8c(u8"保存"))) {
        action = ExportPanelAction::SaveProject;
    }
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::PushID("LoadProjectButton_v4");
    if (ImGui::Button(u8c(u8"読み込み"))) {
        action = ExportPanelAction::LoadProject;
    }
    ImGui::PopID();

    ImGui::Separator();
    ImGui::PushID("PngFolderInput_v4");
    ImGui::InputText(u8c(u8"PNG出力先"), state.exportFolder, sizeof(state.exportFolder));
    ImGui::PopID();
    ImGui::TextDisabled("default: exports/png");
    ImGui::PushID("ExportActivePngButton_v4");
    if (ImGui::Button(u8c(u8"現在フレームPNG"))) {
        action = ExportPanelAction::ExportActivePng;
    }
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::PushID("ExportPngSequenceButton_v4");
    if (ImGui::Button(u8c(u8"PNG連番"))) {
        action = ExportPanelAction::ExportPngSequence;
    }
    ImGui::PopID();

    ImGui::Separator();
    ImGui::PushID("FfmpegPathInput_v4");
    ImGui::InputText("FFmpeg", state.ffmpegPath, sizeof(state.ffmpegPath));
    ImGui::PopID();
    ImGui::TextDisabled("default: ffmpeg / full path is also OK");
    ImGui::PushID("Mp4PathInput_v4");
    ImGui::InputText("MP4", state.mp4Path, sizeof(state.mp4Path));
    ImGui::PopID();
    ImGui::PushID("ExportMp4Button_v4");
    if (ImGui::Button(u8c(u8"MP4書き出し"))) {
        action = ExportPanelAction::ExportMp4;
    }
    ImGui::PopID();
    ImGui::TextDisabled("log: exports/mp4/ffmpeg_last_run.log");

    ImGui::Spacing();
    ImGui::TextUnformatted(u8c(u8"実行結果"));
    ImGui::BeginChild("Export_LastMessage_v4", ImVec2(0.0f, 92.0f), true,
        ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);
    const char* message = lastMessage == nullptr ? "" : lastMessage;
    ImGui::TextWrapped("%s", message);
    ImGui::EndChild();

    ImGui::PopID();
    ImGui::PopID();
    return action;
}

} // namespace perapera::ui
