#pragma once

// このファイルの役割:
// ペンで引いた1本の線を表す。
// 点列、色、太さを保持し、描画キャッシュ更新に使う外接矩形を計算する。

#include <array>
#include <string>
#include <vector>

#include "core/StrokePoint.h"

namespace perapera {

// ストロークをどのブラシエンジンで焼いたかを保持する。
// 既存プロジェクトはSimple扱いにし、MyPaintはPhase 1.5 Step 13以降の追加機能として扱う。
enum class StrokeBrushEngine {
    Simple,
    MyPaint,
};

const char* strokeBrushEngineToString(StrokeBrushEngine engine);
StrokeBrushEngine strokeBrushEngineFromString(const std::string& value);

struct StrokeBounds {
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    bool valid = false;
};

struct Stroke {
    // RGBA。各要素は 0.0〜1.0 を想定する。
    std::array<float, 4> color = {0.05f, 0.05f, 0.05f, 1.0f};

    // キャンバス上の半径px。
    float radiusPx = 4.0f;

    // このストロークを焼き込む時に使うブラシエンジン。
    // JSONでは文字列で保存し、古いデータには影響を出さない。
    StrokeBrushEngine brushEngine = StrokeBrushEngine::Simple;

    std::vector<StrokePoint> points;

    bool empty() const;
    void addPoint(const StrokePoint& point);

    // 点列の外接矩形を返す。
    // ブラシ半径ぶん外側に広げるので、dirty矩形計算にそのまま使える。
    StrokeBounds bounds() const;
};

} // namespace perapera
