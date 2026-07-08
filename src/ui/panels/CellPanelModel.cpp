#include "ui/panels/CellPanelModel.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

namespace perapera::ui {
namespace {

std::string sanitizeIdComponent(const std::string& value, const std::string& fallback)
{
    std::string out;
    out.reserve(value.size());
    for (unsigned char ch : value) {
        if (std::isalnum(ch)) {
            out.push_back(static_cast<char>(std::tolower(ch)));
        } else if (ch == '_' || ch == '-') {
            out.push_back('_');
        }
    }
    if (out.empty()) {
        out = fallback;
    }
    return out;
}

std::string numberedId(const char* prefix, int number)
{
    char buffer[48]{};
    std::snprintf(buffer, sizeof(buffer), "%s_%03d", prefix, std::max(1, number));
    return std::string(buffer);
}

bool layerIdLooksCellScoped(const Layer& layer, const Cell& cell, int cellIndex)
{
    const std::string fallbackCell = numberedId("cell", cellIndex + 1);
    const std::string cellKey = sanitizeIdComponent(cell.id, fallbackCell);
    const std::string prefix = cellKey + "_f";
    return layer.layerId.rfind(prefix, 0) == 0;
}

bool cellIdExists(const Project& project, const std::string& id)
{
    return std::any_of(project.cells.begin(), project.cells.end(), [&](const Cell& cell) {
        return cell.id == id;
    });
}

std::string makeCellId(int number)
{
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "cell_%03d", std::max(1, number));
    return std::string(buffer);
}

} // namespace

std::string expectedLayerIdForCellFrame(const Cell& cell, int cellIndex, int frameIndex, int layerIndex)
{
    const std::string fallbackCell = numberedId("cell", cellIndex + 1);
    const std::string cellKey = sanitizeIdComponent(cell.id, fallbackCell);

    char buffer[96]{};
    std::snprintf(buffer,
                  sizeof(buffer),
                  "%s_f%03d_layer_%03d",
                  cellKey.c_str(),
                  std::max(1, frameIndex + 1),
                  std::max(1, layerIndex + 1));
    return std::string(buffer);
}

bool ensureCellScopedLayerIds(Project& project)
{
    bool changed = false;
    for (int cellIndex = 0; cellIndex < static_cast<int>(project.cells.size()); ++cellIndex) {
        Cell& cell = project.cells[static_cast<std::size_t>(cellIndex)];
        for (int frameIndex = 0; frameIndex < static_cast<int>(cell.frames.size()); ++frameIndex) {
            Frame& frame = cell.frames[static_cast<std::size_t>(frameIndex)];
            for (int layerIndex = 0; layerIndex < static_cast<int>(frame.layers.size()); ++layerIndex) {
                Layer& layer = frame.layers[static_cast<std::size_t>(layerIndex)];
                if (!layerIdLooksCellScoped(layer, cell, cellIndex)) {
                    layer.layerId = expectedLayerIdForCellFrame(cell, cellIndex, frameIndex, layerIndex);
                    layer.touchRevision();
                    changed = true;
                }
            }
        }
    }
    return changed;
}

std::string displayNameForCell(const Cell& cell, int index)
{
    std::string label = cell.name.empty() ? cell.id : cell.name;
    if (label.empty()) {
        label = "Cell " + std::to_string(index + 1);
    }
    if (!cell.category.empty()) {
        label += " [" + cell.category + "]";
    }
    return label;
}

int layerCountForCell(const Cell& cell)
{
    int total = 0;
    for (const Frame& frame : cell.frames) {
        total += static_cast<int>(frame.layers.size());
    }
    return total;
}

int firstFrameLayerCount(const Cell& cell)
{
    if (cell.frames.empty()) {
        return 0;
    }
    return static_cast<int>(cell.frames.front().layers.size());
}

std::string makeUniqueCellId(const Project& project)
{
    int number = static_cast<int>(project.cells.size()) + 1;
    std::string id = makeCellId(number);
    while (cellIdExists(project, id)) {
        ++number;
        id = makeCellId(number);
    }
    return id;
}

void syncCellOrder(Project& project, const std::string& cellId)
{
    if (cellId.empty()) {
        return;
    }
    const auto found = std::find(project.cellOrder.begin(), project.cellOrder.end(), cellId);
    if (found == project.cellOrder.end()) {
        project.cellOrder.push_back(cellId);
    }
}

bool rebuildCellOrderAndZ(Project& project)
{
    bool changed = false;

    if (project.cellOrder.size() != project.cells.size()) {
        changed = true;
    }

    project.cellOrder.clear();
    project.cellOrder.reserve(project.cells.size());

    for (int index = 0; index < static_cast<int>(project.cells.size()); ++index) {
        Cell& cell = project.cells[static_cast<std::size_t>(index)];
        if (cell.zOrder != index) {
            cell.zOrder = index;
            changed = true;
        }
        if (!cell.id.empty()) {
            project.cellOrder.push_back(cell.id);
        }
    }

    return changed;
}

std::string makeDuplicateName(const Cell& source, int copyNumber)
{
    std::string base = source.name.empty() ? source.id : source.name;
    if (base.empty()) {
        base = "Cell";
    }
    return base + " Copy " + std::to_string(std::max(1, copyNumber));
}

void reassignLayerIdsForCell(Cell& cell, int cellIndex)
{
    for (int frameIndex = 0; frameIndex < static_cast<int>(cell.frames.size()); ++frameIndex) {
        Frame& frame = cell.frames[static_cast<std::size_t>(frameIndex)];
        for (int layerIndex = 0; layerIndex < static_cast<int>(frame.layers.size()); ++layerIndex) {
            Layer& layer = frame.layers[static_cast<std::size_t>(layerIndex)];
            layer.layerId = expectedLayerIdForCellFrame(cell, cellIndex, frameIndex, layerIndex);
            layer.touchRevision();
        }
    }
}

} // namespace perapera::ui
