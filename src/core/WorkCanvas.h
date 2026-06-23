#pragma once

// このファイルの役割:
// 作品キャンバスと出力サイズの基本設定を表す。
// UIやレンダリング層に依存しない単純なデータだけを置く。

namespace perapera {

struct WorkCanvas {
    int width = 2560;
    int height = 1440;
};

struct OutputSettings {
    int width = 1920;
    int height = 1080;
    int fps = 24;
    float pixelAspect = 1.0f;
};

struct TimelineSettings {
    int totalFrames = 144;
};

} // namespace perapera
