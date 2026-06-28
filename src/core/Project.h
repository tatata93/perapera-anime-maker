#pragma once

// このファイルの役割:
// 作品全体の状態を表すルートデータ構造を定義する。
// Project はセル集合、キャンバス設定、出力設定、カメラ、音声設定を持つ。
// Phase 2-pre Timesheet Step A: Scene/Cut移行前の仮TimesheetをProject直下に追加する。

#include <string>
#include <vector>

#include "core/Cell.h"
#include "core/Timesheet.h"
#include "core/WorkCanvas.h"

namespace perapera {

struct CameraKey {
    int frame = 0;
    float centerX = 1280.0f;
    float centerY = 720.0f;
    float zoom = 1.0f;
};

struct CameraSettings {
    float centerX = 1280.0f;
    float centerY = 720.0f;
    float zoom = 1.0f;
    bool animationEnabled = false;
    std::vector<CameraKey> keys;
};

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

    // 仮Timesheet。
    // 最終的には Scene/Cut 移行後に Cut 側へ移す想定だが、
    // 現段階では既存の Project::cells 構造を壊さずセルごとの露出タイミングの土台を置く。
    Timesheet timesheet;

    CameraSettings camera;
    AudioSettings audio;
    std::vector<Cell> cells;
    std::vector<std::string> cellOrder;

    static Project createDefault();
    Cell* cellById(const std::string& cellId);
    const Cell* cellById(const std::string& cellId) const;
};

} // namespace perapera
