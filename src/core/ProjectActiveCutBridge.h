#pragma once

// このファイルの役割:
// Phase 2 Step 1-b の移行用ブリッジを定義する。
// まだ Project -> Scene -> Cut -> Cell へ保存形式を移行しない段階で、
// 既存 Project.cells / Project.cellOrder を「暫定 active Cut のセル群」として扱う。
//
// 注意:
// - Project.h / ProjectIO.cpp の保存形式はここでは変更しない。
// - Scene Plate / シーンパネル系は復活させない。
// - UI側はこのブリッジ経由へ段階的に寄せる。

#include <cstddef>
#include <string>
#include <vector>

#include "core/Cell.h"
#include "core/Project.h"

namespace perapera {

struct ActiveCutCellsView {
    std::vector<Cell>* cells = nullptr;
    std::vector<std::string>* cellOrder = nullptr;

    [[nodiscard]] bool valid() const noexcept
    {
        return cells != nullptr && cellOrder != nullptr;
    }

    [[nodiscard]] std::size_t cellCount() const noexcept
    {
        return cells ? cells->size() : 0U;
    }

    [[nodiscard]] std::size_t orderCount() const noexcept
    {
        return cellOrder ? cellOrder->size() : 0U;
    }

    [[nodiscard]] Cell* cellById(const std::string& cellId) const noexcept
    {
        if (!cells) {
            return nullptr;
        }

        for (Cell& cell : *cells) {
            if (cell.id == cellId) {
                return &cell;
            }
        }

        return nullptr;
    }
};

struct ConstActiveCutCellsView {
    const std::vector<Cell>* cells = nullptr;
    const std::vector<std::string>* cellOrder = nullptr;

    [[nodiscard]] bool valid() const noexcept
    {
        return cells != nullptr && cellOrder != nullptr;
    }

    [[nodiscard]] std::size_t cellCount() const noexcept
    {
        return cells ? cells->size() : 0U;
    }

    [[nodiscard]] std::size_t orderCount() const noexcept
    {
        return cellOrder ? cellOrder->size() : 0U;
    }

    [[nodiscard]] const Cell* cellById(const std::string& cellId) const noexcept
    {
        if (!cells) {
            return nullptr;
        }

        for (const Cell& cell : *cells) {
            if (cell.id == cellId) {
                return &cell;
            }
        }

        return nullptr;
    }
};

[[nodiscard]] inline ActiveCutCellsView activeCutCells(Project& project) noexcept
{
    return ActiveCutCellsView{&project.cells, &project.cellOrder};
}

[[nodiscard]] inline ConstActiveCutCellsView activeCutCells(const Project& project) noexcept
{
    return ConstActiveCutCellsView{&project.cells, &project.cellOrder};
}

[[nodiscard]] inline bool activeCutContainsCell(const Project& project, const std::string& cellId) noexcept
{
    return activeCutCells(project).cellById(cellId) != nullptr;
}

inline Cell* ensureActiveCutCell(Project& project, Cell cell)
{
    if (cell.id.empty()) {
        return nullptr;
    }

    if (Cell* existing = activeCutCells(project).cellById(cell.id)) {
        return existing;
    }

    const std::string newCellId = cell.id;
    project.cells.push_back(std::move(cell));

    bool alreadyOrdered = false;
    for (const std::string& orderedId : project.cellOrder) {
        if (orderedId == newCellId) {
            alreadyOrdered = true;
            break;
        }
    }

    if (!alreadyOrdered) {
        project.cellOrder.push_back(newCellId);
    }

    return activeCutCells(project).cellById(newCellId);
}

inline bool ensureActiveCutCellOrder(Project& project, const std::string& cellId)
{
    if (cellId.empty()) {
        return false;
    }

    if (!activeCutContainsCell(project, cellId)) {
        return false;
    }

    for (const std::string& orderedId : project.cellOrder) {
        if (orderedId == cellId) {
            return false;
        }
    }

    project.cellOrder.push_back(cellId);
    return true;
}

} // namespace perapera
