#pragma once

// このファイルの役割:
// Frame/Cellのストローク点列をCPU上でラスタライズし、PNG画像として書き出す入口を定義する。

#include <filesystem>
#include <string>

#include "core/Cell.h"
#include "core/Frame.h"

namespace perapera {

class PngExporter {
public:
    static bool exportFrame(const Frame& frame,
                            const std::filesystem::path& outputPath,
                            int width,
                            int height,
                            std::string* errorMessage = nullptr);

    static bool exportFrameSequence(const Cell& cell,
                                    const std::filesystem::path& outputFolder,
                                    int width,
                                    int height,
                                    std::string* errorMessage = nullptr);
};

} // namespace perapera
