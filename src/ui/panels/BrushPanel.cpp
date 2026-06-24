// このファイルの役割:
// ブラシ設定パネルの実装。
// Phase 1.5 Step 1ではlibmypaintをまだ導入せず、描き味調整UIだけを追加する。

#include "ui/panels/BrushPanel.h"

#include <algorithm>

#include <imgui.h>

namespace perapera::ui {
namespace {

const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

} // namespace

const char* brushPresetLabel(BrushPreset preset)
{
    switch (preset) {
    case BrushPreset::Pen:
        return u8c(u8"ペン");
    case BrushPreset::Pencil:
        return u8c(u8"鉛筆");
    case BrushPreset::Watercolor:
        return u8c(u8"水彩");
    case BrushPreset::Airbrush:
        return u8c(u8"エアブラシ");
    }
    return u8c(u8"ペン");
}

void applyBrushPreset(BrushSettings& settings, BrushPreset preset)
{
    settings.preset = preset;
    settings.tool = ToolKind::Brush;

    switch (preset) {
    case BrushPreset::Pen:
        settings.radiusPx = 4.0f;
        settings.opacity = 1.0f;
        settings.hardness = 1.0f;
        settings.spacing = 0.22f;
        settings.stabilizer = 0.05f;
        settings.entryFade = 0.0f;
        settings.exitFade = 0.0f;
        settings.pressureToSize = 0.0f;
        settings.pressureToOpacity = 0.0f;
        settings.watercolorBleed = 0.0f;
        settings.colorMix = 0.0f;
        settings.dryRate = 1.0f;
        break;
    case BrushPreset::Pencil:
        settings.radiusPx = 3.0f;
        settings.opacity = 0.72f;
        settings.hardness = 0.65f;
        settings.spacing = 0.18f;
        settings.stabilizer = 0.12f;
        settings.entryFade = 0.10f;
        settings.exitFade = 0.18f;
        settings.pressureToSize = 0.25f;
        settings.pressureToOpacity = 0.35f;
        settings.watercolorBleed = 0.0f;
        settings.colorMix = 0.0f;
        settings.dryRate = 1.0f;
        break;
    case BrushPreset::Watercolor:
        settings.radiusPx = 9.0f;
        settings.opacity = 0.55f;
        settings.hardness = 0.35f;
        settings.spacing = 0.16f;
        settings.stabilizer = 0.18f;
        settings.entryFade = 0.12f;
        settings.exitFade = 0.25f;
        settings.pressureToSize = 0.35f;
        settings.pressureToOpacity = 0.45f;
        settings.watercolorBleed = 0.45f;
        settings.colorMix = 0.35f;
        settings.dryRate = 0.55f;
        break;
    case BrushPreset::Airbrush:
        settings.radiusPx = 18.0f;
        settings.opacity = 0.32f;
        settings.hardness = 0.18f;
        settings.spacing = 0.12f;
        settings.stabilizer = 0.0f;
        settings.entryFade = 0.0f;
        settings.exitFade = 0.0f;
        settings.pressureToSize = 0.15f;
        settings.pressureToOpacity = 0.55f;
        settings.watercolorBleed = 0.0f;
        settings.colorMix = 0.0f;
        settings.dryRate = 1.0f;
        break;
    }
}

void drawBrushPanel(BrushSettings& settings)
{
    ImGui::TextUnformatted(u8c(u8"ブラシ"));
    ImGui::Separator();

    int tool = 0;
    if (settings.tool == ToolKind::Eraser) {
        tool = 1;
    } else if (settings.tool == ToolKind::FloodFill) {
        tool = 2;
    }
    if (ImGui::RadioButton(u8c(u8"ブラシ"), tool == 0)) {
        tool = 0;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton(u8c(u8"消しゴム"), tool == 1)) {
        tool = 1;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton(u8c(u8"バケツ"), tool == 2)) {
        tool = 2;
    }
    if (tool == 0) {
        settings.tool = ToolKind::Brush;
    } else if (tool == 1) {
        settings.tool = ToolKind::Eraser;
    } else {
        settings.tool = ToolKind::FloodFill;
    }

    ImGui::TextDisabled(u8c(u8"B:ブラシ  E:消しゴム  G:バケツ"));
    ImGui::TextDisabled(u8c(u8"Engine: SimpleBrushEngine / libmypaintは次Step"));

    ImGui::TextUnformatted(u8c(u8"プリセット"));
    if (ImGui::Button(u8c(u8"ペン"))) {
        applyBrushPreset(settings, BrushPreset::Pen);
    }
    ImGui::SameLine();
    if (ImGui::Button(u8c(u8"鉛筆"))) {
        applyBrushPreset(settings, BrushPreset::Pencil);
    }
    if (ImGui::Button(u8c(u8"水彩"))) {
        applyBrushPreset(settings, BrushPreset::Watercolor);
    }
    ImGui::SameLine();
    if (ImGui::Button(u8c(u8"エアブラシ"))) {
        applyBrushPreset(settings, BrushPreset::Airbrush);
    }
    ImGui::TextDisabled("Preset: %s", brushPresetLabel(settings.preset));

    ImGui::SliderFloat(u8c(u8"サイズ"), &settings.radiusPx, 1.0f, 64.0f, "%.1f px");
    ImGui::SliderFloat(u8c(u8"不透明度"), &settings.opacity, 0.05f, 1.0f, "%.2f");
    ImGui::SliderFloat(u8c(u8"硬さ"), &settings.hardness, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat(u8c(u8"間隔"), &settings.spacing, 0.05f, 1.0f, "%.2f");
    ImGui::SliderFloat(u8c(u8"手ぶれ補正"), &settings.stabilizer, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat(u8c(u8"入り"), &settings.entryFade, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat(u8c(u8"抜き"), &settings.exitFade, 0.0f, 1.0f, "%.2f");

    if (ImGui::TreeNode(u8c(u8"筆圧マッピング"))) {
        ImGui::SliderFloat(u8c(u8"筆圧→サイズ"), &settings.pressureToSize, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat(u8c(u8"筆圧→不透明度"), &settings.pressureToOpacity, 0.0f, 1.0f, "%.2f");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode(u8c(u8"水彩パラメータ"))) {
        ImGui::SliderFloat(u8c(u8"にじみ"), &settings.watercolorBleed, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat(u8c(u8"色混ぜ"), &settings.colorMix, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat(u8c(u8"乾き速度"), &settings.dryRate, 0.0f, 1.0f, "%.2f");
        ImGui::TextDisabled(u8c(u8"水彩の実描画はlibmypaint導入後に接続"));
        ImGui::TreePop();
    }

    settings.radiusPx = std::clamp(settings.radiusPx, 1.0f, 64.0f);
    settings.opacity = std::clamp(settings.opacity, 0.05f, 1.0f);
    settings.hardness = std::clamp(settings.hardness, 0.0f, 1.0f);
    settings.spacing = std::clamp(settings.spacing, 0.05f, 1.0f);
    settings.stabilizer = std::clamp(settings.stabilizer, 0.0f, 1.0f);
    settings.entryFade = std::clamp(settings.entryFade, 0.0f, 1.0f);
    settings.exitFade = std::clamp(settings.exitFade, 0.0f, 1.0f);
    settings.pressureToSize = std::clamp(settings.pressureToSize, 0.0f, 1.0f);
    settings.pressureToOpacity = std::clamp(settings.pressureToOpacity, 0.0f, 1.0f);
    settings.watercolorBleed = std::clamp(settings.watercolorBleed, 0.0f, 1.0f);
    settings.colorMix = std::clamp(settings.colorMix, 0.0f, 1.0f);
    settings.dryRate = std::clamp(settings.dryRate, 0.0f, 1.0f);
    settings.fillTolerance = std::clamp(settings.fillTolerance, 0, 255);
    settings.fillGapClosePx = std::clamp(settings.fillGapClosePx, 0, 6);

    if (settings.tool == ToolKind::FloodFill) {
        ImGui::Separator();
        ImGui::TextUnformatted(u8c(u8"バケツ塗り"));
        ImGui::SliderInt(u8c(u8"許容範囲"), &settings.fillTolerance, 0, 255);
        ImGui::SliderInt(u8c(u8"隙間閉じpx"), &settings.fillGapClosePx, 0, 6);
        ImGui::TextDisabled(u8c(u8"Paintレイヤー上でクリックして塗る"));
    }

    ImGui::ColorEdit4(u8c(u8"色"), settings.color.data(), ImGuiColorEditFlags_NoInputs);
}

} // namespace perapera::ui
