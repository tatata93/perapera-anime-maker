// src/ui/panels/ExportPanel.cpp

#include "ui/panels/ExportPanel.h"

#include "imgui.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>

namespace perapera
{
    namespace
    {
        void copyStringToInputBuffer(
            const std::string& source,
            char* buffer,
            std::size_t bufferSize
        )
        {
            if (buffer == nullptr || bufferSize == 0)
            {
                return;
            }

            const std::size_t copySize = std::min(
                source.size(),
                bufferSize - 1
            );

            std::copy_n(source.c_str(), copySize, buffer);
            buffer[copySize] = '\0';
        }

        void drawResultMessage(
            bool succeeded,
            const std::string& message
        )
        {
            if (message.empty())
            {
                return;
            }

            ImGui::TextColored(
                succeeded
                    ? ImVec4(0.45f, 1.0f, 0.55f, 1.0f)
                    : ImVec4(1.0f, 0.45f, 0.25f, 1.0f),
                "%s",
                message.c_str()
            );
        }
    }

    ExportPanelActions ExportPanel::draw(
        int framesPerSecond,
        int plannedPngSequenceImageCount
    )
    {
        ExportPanelActions actions;

        ImGui::Begin("出力");

        ImGui::Text("PNG保存");
        ImGui::Checkbox(
            "透明背景で保存",
            &transparentBackground_
        );

        if (ImGui::Button("現在の撮影フレームをPNG保存"))
        {
            actions.exportCurrentPng = true;
        }

        drawResultMessage(
            currentPngSucceeded_,
            currentPngMessage_
        );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("PNG連番保存");
        ImGui::Text("保持コマ数を反映して全フレームを書き出します。");
        ImGui::Text(
            "出力予定枚数: %d",
            plannedPngSequenceImageCount
        );

        if (ImGui::Button("全フレームをPNG連番保存"))
        {
            actions.exportPngSequence = true;
        }

        drawResultMessage(
            pngSequenceSucceeded_,
            pngSequenceMessage_
        );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("MP4動画保存 / 外部FFmpeg");
        ImGui::Text(
            "PNG連番をFFmpegでH.264のMP4へ変換します。FPS: %d",
            framesPerSecond
        );

        char ffmpegPathBuffer[512] = {};
        copyStringToInputBuffer(
            ffmpegSettings_.executablePath.string(),
            ffmpegPathBuffer,
            sizeof(ffmpegPathBuffer)
        );

        ImGui::SetNextItemWidth(420.0f);
        if (ImGui::InputText(
            "ffmpeg.exeのパス",
            ffmpegPathBuffer,
            sizeof(ffmpegPathBuffer)
        ))
        {
            ffmpegSettings_.executablePath =
                std::filesystem::path(ffmpegPathBuffer);
        }

        ImGui::SetNextItemWidth(180.0f);
        ImGui::SliderInt(
            "動画品質 CRF",
            &ffmpegSettings_.constantRateFactor,
            0,
            51
        );
        ImGui::Text("CRFは小さいほど高画質です。通常は18～23を使用します。");

        const char* presetItems[] = {
            "ultrafast",
            "fast",
            "medium",
            "slow"
        };
        int presetIndex = 2;

        for (int index = 0; index < 4; ++index)
        {
            if (ffmpegSettings_.preset == presetItems[index])
            {
                presetIndex = index;
                break;
            }
        }

        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::Combo(
            "圧縮速度",
            &presetIndex,
            presetItems,
            4
        ))
        {
            ffmpegSettings_.preset = presetItems[presetIndex];
        }

        ImGui::Checkbox(
            "動画変換後も一時PNGを残す",
            &keepVideoPngFrames_
        );

        if (ImGui::Button("全フレームをMP4動画保存"))
        {
            actions.exportMp4 = true;
        }

        drawResultMessage(
            videoSucceeded_,
            videoMessage_
        );

        if (!videoLog_.empty()
            && ImGui::TreeNode("FFmpeg実行ログ"))
        {
            ImGui::BeginChild(
                "FfmpegLog",
                ImVec2(0.0f, 180.0f),
                true,
                ImGuiWindowFlags_HorizontalScrollbar
            );
            ImGui::TextUnformatted(videoLog_.c_str());
            ImGui::EndChild();
            ImGui::TreePop();
        }

        ImGui::End();
        return actions;
    }

    bool ExportPanel::transparentBackground() const
    {
        return transparentBackground_;
    }

    FfmpegSettings& ExportPanel::ffmpegSettings()
    {
        return ffmpegSettings_;
    }

    bool ExportPanel::keepVideoPngFrames() const
    {
        return keepVideoPngFrames_;
    }

    void ExportPanel::setCurrentPngResult(
        bool succeeded,
        const std::string& message
    )
    {
        currentPngSucceeded_ = succeeded;
        currentPngMessage_ = message;
    }

    void ExportPanel::setPngSequenceResult(
        bool succeeded,
        const std::string& message
    )
    {
        pngSequenceSucceeded_ = succeeded;
        pngSequenceMessage_ = message;
    }

    void ExportPanel::setVideoResult(
        bool succeeded,
        const std::string& message,
        const std::string& log
    )
    {
        videoSucceeded_ = succeeded;
        videoMessage_ = message;
        videoLog_ = log;
    }
}
