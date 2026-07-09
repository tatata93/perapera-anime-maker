#pragma once

#include "core/Cell.h"

namespace perapera {

[[nodiscard]] CellPlacement resolveCellPlacementAtFrame(const Cell& cell, int timelineFrame);

} // namespace perapera
