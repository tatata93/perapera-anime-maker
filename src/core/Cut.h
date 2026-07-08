// このファイルの役割:
// 正式タイムシートが接続されるCut単位の最小モデルを定義する。
// 既存Projectへまだ接続しない。Step 1ではデータの意味だけを先に固定する。

#pragma once

#include "core/CameraSettings.h"
#include "core/Timesheet.h"

#include <string>
#include <vector>

namespace perapera {

struct Cut {
    std::string id;
    std::string name;
    int frameRate = 24;
    int totalFrames = 144;

    // タイムシートはProject直下ではなくCut単位に持つ。
    Timesheet timesheet;
    bool hasCamera = false;
    CameraSettings camera;

    // 将来、表示・撮影順とタイムシート列順を接続するためのキー。
    std::vector<std::string> cellZOrderKeys;
};

} // namespace perapera
