#pragma once

// このファイルの役割:
// Frame/Cellのストローク点列をCPU上でラスタライズし、PNG画像として書き出す入口を定義する。

#include <filesystem>
#include <string>

#include "core/Cell.h"
#include "core/Frame.h"

namespace perapera {

// Phase 1.5 Step 15: PNG/MP4書き出し時のレイヤー選別モード。
enum class ExportMode {
    Composite,
    LineTest,
    ColorTrace,
    LineOnly,
};

const char* exportModeToString(ExportMode mode);
const char* exportModeDisplayName(ExportMode mode);

class PngExporter {
public:
    static bool exportFrame(const Frame& frame,
                            const std::filesystem::path& outputPath,
                            int width,
                            int height,
                            ExportMode mode = ExportMode::Composite,
                            std::string* errorMessage = nullptr);

    static bool exportFrameSequence(const Cell& cell,
                                    const std::filesystem::path& outputFolder,
                                    int width,
                                    int height,
                                    ExportMode mode = ExportMode::Composite,
                                    std::string* errorMessage = nullptr);
};

} // namespace perapera
