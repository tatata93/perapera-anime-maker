#pragma once

// このファイルの役割:
// ペンで引いた1本の線、またはバケツ塗りで作ったFillStrokeを表す。
// Simple/MyPaintは点列を正データにし、Fillは1chマスクを正データにする。

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "core/StrokePoint.h"

namespace perapera {

// ストロークをどのブラシエンジンで焼いたかを保持する。
// Fillはバケツ塗り専用で、pointsを使わずbitmapを直接CanvasBitmapへ焼く。
enum class StrokeBrushEngine {
    Simple,
    MyPaint,
    Fill,
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

    // キャンバス上の半径px。Fillでは使わない。
    float radiusPx = 4.0f;

    // このストロークを焼き込む時に使うブラシエンジン。
    StrokeBrushEngine brushEngine = StrokeBrushEngine::Simple;

    // Phase 1.5 Step 14: ブラシ設定をストロークへ保存する。
    // これにより、保存/読み込み後やキャッシュ再構築後もMyPaintの描き味が復元される。
    float opacity = 1.0f;
    float hardness = 1.0f;
    float spacing = 0.25f;
    float pressureToSize = 0.0f;
    float pressureToOpacity = 0.0f;
    float watercolorBleed = 0.0f;
    float colorMix = 0.0f;
    float dryRate = 1.0f;

    // Simple / MyPaint 用の点列。Fillでは空のまま使わない。
    std::vector<StrokePoint> points;

    // Fill エンジンのときだけ使う。
    // bitmapX/Y を左上原点とする bitmapWidth × bitmapHeight の 1ch マスク。
    // マスク値は 0=塗らない / 255=塗る。古い作業中データの 1 も塗りとして扱う。
    // points は空のまま使わない。
    std::vector<std::uint8_t> bitmap;
    int bitmapX = 0;
    int bitmapY = 0;
    int bitmapWidth = 0;
    int bitmapHeight = 0;

    bool empty() const;
    void addPoint(const StrokePoint& point);

    // 点列の外接矩形を返す。
    // Fillは保存済みマスクの外接矩形を返す。
    StrokeBounds bounds() const;
};

} // namespace perapera
