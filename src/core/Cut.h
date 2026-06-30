// このファイルの役割:
// 正式タイムシートが接続されるCut単位の最小モデルを定義する。
// 既存Projectへまだ接続しない。Step 1ではデータの意味だけを先に固定する。

#pragma once

#include "core/ScenePlate.h"
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

    // 絵コンテ、レイアウト、仮背景、完成背景など。
    // セル列とは別管理し、作画セル・タイムシート列を汚さない。
    ScenePlateStack scenePlates;

    // 将来、表示・撮影順とタイムシート列順を接続するためのキー。
    std::vector<std::string> cellZOrderKeys;
};

} // namespace perapera
