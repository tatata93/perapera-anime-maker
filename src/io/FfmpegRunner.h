#pragma once

// このファイルの役割:
// 外部FFmpegをコマンド実行して、PNG連番からMP4を作る入口を定義する。
// FFmpegは本体に組み込まず、外部ツールとして呼び出す。

#include <filesystem>
#include <string>

namespace perapera {

class FfmpegRunner {
public:
    static std::string buildPngSequenceCommand(const std::string& ffmpegPath,
                                               const std::filesystem::path& inputPattern,
                                               int fps,
                                               const std::filesystem::path& outputPath);

    static bool pngSequenceToMp4(const std::string& ffmpegPath,
                                 const std::filesystem::path& inputPattern,
                                 int fps,
                                 const std::filesystem::path& outputPath,
                                 std::string* errorMessage = nullptr);
};

} // namespace perapera
