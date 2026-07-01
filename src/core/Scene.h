// このファイルの役割:
// final_spec_v6 Phase 2 Step 1-a の Scene / Cut 最小モデルを定義する。
// まだ Project 保存形式や UI には接続しない。
// Scene Plate / シーンパネルではなく、通常 Cell + Timesheet 管理へ進むための土台。

#pragma once

#include "core/Cut.h"

#include <cstddef>
#include <string>
#include <vector>

namespace perapera {

struct Scene {
    std::string id = "scene-001";
    std::string name = "Scene 1";
    std::string memo;

    // Phase 2 Step 1-a では、Cut の所有先だけを明確にする。
    // 既存 Project.cells はまだ移動しない。
    std::vector<Cut> cuts;

    // 将来の scene.json 用。空の場合は cuts の順序を使う。
    std::vector<std::string> cutOrder;
};

inline std::size_t normalizeSceneIndex(std::size_t index, std::size_t sceneCount) noexcept
{
    if (sceneCount == 0) {
        return 0;
    }
    return index < sceneCount ? index : sceneCount - 1;
}

inline std::size_t normalizeCutIndex(std::size_t index, std::size_t cutCount) noexcept
{
    if (cutCount == 0) {
        return 0;
    }
    return index < cutCount ? index : cutCount - 1;
}

inline Cut* activeCut(Scene& scene, std::size_t activeCutIndex) noexcept
{
    if (scene.cuts.empty()) {
        return nullptr;
    }
    return &scene.cuts[normalizeCutIndex(activeCutIndex, scene.cuts.size())];
}

inline const Cut* activeCut(const Scene& scene, std::size_t activeCutIndex) noexcept
{
    if (scene.cuts.empty()) {
        return nullptr;
    }
    return &scene.cuts[normalizeCutIndex(activeCutIndex, scene.cuts.size())];
}

inline std::vector<std::string> resolvedCutOrder(const Scene& scene)
{
    if (!scene.cutOrder.empty()) {
        return scene.cutOrder;
    }

    std::vector<std::string> order;
    order.reserve(scene.cuts.size());

    for (const Cut& cut : scene.cuts) {
        order.push_back(cut.id);
    }

    return order;
}

} // namespace perapera
