
// src/ui/panels/ExportPanel.h
//
// PNG/MP4出力の設定と結果表示を担当するUIパネルです。
// 実際のファイル出力はDrawingCanvasPanelへ要求として返します。

#pragma once

#include "export/FfmpegExporter.h"

#include <string>

namespace perapera
{
    struct ExportPanelActions
    {
        bool exportCurrentPng = false;
        bool exportPngSequence = false;
        bool exportMp4 = false;
    };

    class ExportPanel
    {
    public:
        ExportPanelActions draw(
            int framesPerSecond,
            int plannedPngSequenceImageCount
        );

        bool transparentBackground() const;

        FfmpegSettings& ffmpegSettings();

        bool keepVideoPngFrames() const;

        void setCurrentPngResult(
            bool succeeded,
            const std::string& message
        );

        void setPngSequenceResult(
            bool succeeded,
            const std::string& message
        );

        void setVideoResult(
            bool succeeded,
            const std::string& message,
            const std::string& log
        );

    private:
        bool transparentBackground_ = false;

        std::string currentPngMessage_;
        bool currentPngSucceeded_ = false;

        std::string pngSequenceMessage_;
        bool pngSequenceSucceeded_ = false;

        FfmpegSettings ffmpegSettings_;
        bool keepVideoPngFrames_ = false;

        std::string videoMessage_;
        std::string videoLog_;
        bool videoSucceeded_ = false;
    };
}
