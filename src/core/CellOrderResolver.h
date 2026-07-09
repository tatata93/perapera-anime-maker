#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/Project.h"

namespace perapera {

namespace detail {

[[nodiscard]] inline bool containsCellOrderIndex(const std::vector<int>& indices, int index)
{
    for (const int current : indices) {
        if (current == index) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] inline int cellIndexById(const Project& project, const std::string& cellId)
{
    if (cellId.empty()) {
        return -1;
    }
    for (int index = 0; index < static_cast<int>(project.cells.size()); ++index) {
        if (project.cells[static_cast<std::size_t>(index)].id == cellId) {
            return index;
        }
    }
    return -1;
}

} // namespace detail

[[nodiscard]] inline std::vector<int> resolvedProjectCellOrderIndices(const Project& project)
{
    std::vector<int> indices;
    indices.reserve(project.cells.size());

    for (const std::string& cellId : project.cellOrder) {
        const int index = detail::cellIndexById(project, cellId);
        if (index >= 0 && !detail::containsCellOrderIndex(indices, index)) {
            indices.push_back(index);
        }
    }

    for (int index = 0; index < static_cast<int>(project.cells.size()); ++index) {
        if (!detail::containsCellOrderIndex(indices, index)) {
            indices.push_back(index);
        }
    }

    return indices;
}

} // namespace perapera
