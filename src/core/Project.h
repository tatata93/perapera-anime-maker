#pragma once

// このファイルの役割:
// 作品全体の状態を表すルートデータ構造を定義する。
// Project はセル集合、キャンバス設定、出力設定、カメラ、音声設定を持つ。

#include <string>
#include <vector>

#include "core/CameraSettings.h"
#include "core/Cell.h"
#include "core/WorkCanvas.h"

namespace perapera {

struct AudioSettings {
    bool enabled = false;
    std::string filePath;
    int startFrame = 0;
};

struct Project {
    std::string version = "1.0";
    std::string name = "作品タイトル";
    WorkCanvas canvas;
    OutputSettings output;
    TimelineSettings timeline;
    CameraSettings camera;
    AudioSettings audio;
    std::vector<Cell> cells;
    std::vector<std::string> cellOrder;

    static Project createDefault();

    Cell* cellById(const std::string& cellId);
    const Cell* cellById(const std::string& cellId) const;
};

} // namespace perapera
