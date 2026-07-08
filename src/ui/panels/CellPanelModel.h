#pragma once

#include <string>

#include "core/Cell.h"
#include "core/Project.h"

namespace perapera::ui {

std::string expectedLayerIdForCellFrame(const Cell& cell, int cellIndex, int frameIndex, int layerIndex);
bool ensureCellScopedLayerIds(Project& project);
std::string displayNameForCell(const Cell& cell, int index);
int layerCountForCell(const Cell& cell);
int firstFrameLayerCount(const Cell& cell);
std::string makeUniqueCellId(const Project& project);
void syncCellOrder(Project& project, const std::string& cellId);
bool rebuildCellOrderAndZ(Project& project);
std::string makeDuplicateName(const Cell& source, int copyNumber);
void reassignLayerIdsForCell(Cell& cell, int cellIndex);

} // namespace perapera::ui
