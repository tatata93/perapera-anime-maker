// src/export/FfmpegExporter.h
//
// 外部FFmpegを使ってPNG連番をMP4へ変換します。
// FFmpeg本体はアプリへ同梱しません。

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace perapera
{
    struct FfmpegSettings
    {
        std::filesystem::path executablePath = "ffmpeg.exe";
        int framesPerSecond = 24;
        int constantRateFactor = 20;
        std::string preset = "medium";
        bool overwriteOutput = true;
    };

    struct FfmpegExportResult
    {
        bool succeeded = false;
        int exitCode = -1;
        std::string command;
        std::string log;
        std::string errorMessage;
    };

    class FfmpegExporter
    {
    public:
        static std::vector<std::string> buildArguments(
            const std::filesystem::path& inputPattern,
            const std::filesystem::path& outputPath,
            const FfmpegSettings& settings
        );

        static std::string formatCommandForDisplay(
            const std::filesystem::path& executablePath,
            const std::vector<std::string>& arguments
        );

        static FfmpegExportResult exportPngSequenceToMp4(
            const std::filesystem::path& inputPattern,
            const std::filesystem::path& outputPath,
            const FfmpegSettings& settings
        );
    };
}
