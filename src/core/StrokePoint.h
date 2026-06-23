#pragma once

// このファイルの役割:
// 1つのストロークを構成する点を表す。
// 座標はキャンバス上のピクセル座標、pressure は筆圧として扱う。

namespace perapera {

struct StrokePoint {
    float x = 0.0f;
    float y = 0.0f;

    // 0.0〜1.0を想定する。
    // マウス入力など筆圧がない入力では 1.0 として扱う。
    float pressure = 1.0f;
};

} // namespace perapera
