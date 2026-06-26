#pragma once

// このファイルの役割:
// 1フレーム内の1枚の作画レイヤーを表す。
// レイヤーはストローク集合を持つが、SDL_Textureなどの描画キャッシュは持たない。

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "core/Stroke.h"

namespace perapera {

enum class LayerType {
    Normal,
    ColorTrace,
    Paint,
    ShadowGuide,
    Rough,
};

const char* layerTypeToString(LayerType type);
const char* layerTypeDisplayName(LayerType type);
LayerType layerTypeFromString(std::string_view value);

struct Layer {
    std::string layerId = "layer_001";
    std::string name = "線画";
    bool visible = true;
    float opacity = 1.0f;
    std::string blendMode = "normal";
    LayerType type = LayerType::Normal;
    std::vector<Stroke> strokes;

    // 表示キャッシュ用の軽量revision。
    // ストローク追加/削除/編集やレイヤー属性変更時にインクリメントする。
    // 保存/読み込みでは永続化せず、読み込み直後は0でよい。
    // CanvasRenderer は全StrokePointを毎フレーム走査せず、この値と軽量メタ情報だけを見る。
    std::uint64_t revisionCounter = 0U;

    void touchRevision() noexcept { ++revisionCounter; }

    static Layer createDefault(int layerNumber);
};

} // namespace perapera
