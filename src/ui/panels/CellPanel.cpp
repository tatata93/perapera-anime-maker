// This file's role: compact Cell management UI for selecting, creating, duplicating, editing, deleting, ordering, and displaying cells. v1.9e moves the timesheet editor into a detached ImGui window so the right Cell panel stays compact.
#include "ui/panels/CellPanel.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <string>
#include <utility>

#include <imgui.h>

namespace perapera::ui {
namespace {

struct CategoryOption {
    const char* label;
    const char* value;
};

constexpr std::array<CategoryOption, 6> kCategoryOptions{{
    {"Character", "character"},
    {"Background", "background"},
    {"Book", "book"},
    {"Effect", "effect"},
    {"CameraGuide", "camera_guide"},
    {"Other", "other"},
}};

CellDisplayMode gDisplayMode = CellDisplayMode::VisibleCells;
int gSoloCellIndex = -1;
bool gTimesheetWindowOpen = true;

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

bool layerIdLooksCellScoped(const Layer& layer, const Cell& cell, int cellIndex)
{
    const std::string fallbackCell = numberedId("cell", cellIndex + 1);
    const std::string cellKey = sanitizeIdComponent(cell.id, fallbackCell);
    const std::string prefix = cellKey + "_f";
    return layer.layerId.rfind(prefix, 0) == 0;
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

int categoryIndexFromValue(const std::string& value)
{
    for (int i = 0; i < static_cast<int>(kCategoryOptions.size()); ++i) {
        if (value == kCategoryOptions[static_cast<std::size_t>(i)].value) {
            return i;
        }
    }
    return 0;
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

const Cell* cellByIdInProject(const Project& project, const std::string& cellId)
{
    const auto found = std::find_if(project.cells.begin(), project.cells.end(), [&](const Cell& cell) {
        return cell.id == cellId;
    });
    return found == project.cells.end() ? nullptr : &(*found);
}

Cell* cellByIdInProject(Project& project, const std::string& cellId)
{
    auto found = std::find_if(project.cells.begin(), project.cells.end(), [&](Cell& cell) {
        return cell.id == cellId;
    });
    return found == project.cells.end() ? nullptr : &(*found);
}

bool removeTimesheetCell(Project& project, const std::string& cellId)
{
    if (cellId.empty()) {
        return false;
    }

    bool changed = false;
    for (TimesheetFrame& frame : project.timesheet.frames) {
        const auto oldSize = frame.cellExposures.size();
        frame.cellExposures.erase(std::remove_if(frame.cellExposures.begin(),
                                                 frame.cellExposures.end(),
                                                 [&](const TimesheetCellExposure& exposure) {
                                                     return exposure.cellId == cellId;
                                                 }),
                                  frame.cellExposures.end());
        changed = changed || frame.cellExposures.size() != oldSize;
    }
    return changed;
}

int safeTimelineFrameCount(const Project& project)
{
    return std::max(1, project.timeline.totalFrames);
}

int frameCountForCell(const Cell& cell)
{
    return std::max(1, static_cast<int>(cell.frames.size()));
}

bool normalizeTimesheetForProject(Project& project)
{
    bool changed = false;

    const int requestedFrameCount = safeTimelineFrameCount(project);
    if (project.timesheet.totalFrames != requestedFrameCount ||
        static_cast<int>(project.timesheet.frames.size()) != requestedFrameCount) {
        project.timesheet.ensureFrameCount(requestedFrameCount);
        changed = true;
    }

    for (TimesheetFrame& frame : project.timesheet.frames) {
        for (auto it = frame.cellExposures.begin(); it != frame.cellExposures.end();) {
            if (cellByIdInProject(project, it->cellId) == nullptr) {
                it = frame.cellExposures.erase(it);
                changed = true;
            } else {
                ++it;
            }
        }
    }

    for (const Cell& cell : project.cells) {
        if (cell.id.empty()) {
            continue;
        }
        for (TimesheetFrame& frame : project.timesheet.frames) {
            if (frame.exposureForCell(cell.id) == nullptr) {
                TimesheetCellExposure exposure;
                exposure.cellId = cell.id;
                exposure.drawingFrameIndex = 0;
                exposure.kind = TimesheetExposureKind::Hold;
                frame.cellExposures.push_back(std::move(exposure));
                changed = true;
            }
        }
    }

    for (TimesheetFrame& frame : project.timesheet.frames) {
        frame.timelineFrame = std::clamp(frame.timelineFrame, 0, requestedFrameCount - 1);
        for (TimesheetCellExposure& exposure : frame.cellExposures) {
            const Cell* cell = cellByIdInProject(project, exposure.cellId);
            if (cell == nullptr) {
                continue;
            }
            const int maxDrawingFrame = frameCountForCell(*cell) - 1;
            const int clamped = std::clamp(exposure.drawingFrameIndex, 0, maxDrawingFrame);
            if (exposure.drawingFrameIndex != clamped) {
                exposure.drawingFrameIndex = clamped;
                changed = true;
            }
        }
    }

    return changed;
}

TimesheetCellExposure* ensureTimesheetExposure(Project& project, int timelineFrame, const std::string& cellId)
{
    project.timesheet.ensureFrameCount(safeTimelineFrameCount(project));
    project.timesheet.ensureCell(cellId, 0);

    TimesheetFrame* frame = project.timesheet.frameOrNull(timelineFrame);
    if (frame == nullptr) {
        return nullptr;
    }

    TimesheetCellExposure* exposure = frame->exposureForCell(cellId);
    if (exposure != nullptr) {
        return exposure;
    }

    TimesheetCellExposure newExposure;
    newExposure.cellId = cellId;
    newExposure.drawingFrameIndex = 0;
    newExposure.kind = TimesheetExposureKind::Hold;
    frame->cellExposures.push_back(std::move(newExposure));
    return frame->exposureForCell(cellId);
}

void markTimesheetChanged(CellPanelResult& result)
{
    result.timesheetChanged = true;
    result.displayChanged = true;
    result.projectStructureChanged = true;
}

const char* exposureKindLabel(TimesheetExposureKind kind)
{
    switch (kind) {
    case TimesheetExposureKind::Null:
        return "Null";
    case TimesheetExposureKind::Hold:
        return "Hold";
    case TimesheetExposureKind::Key:
        return "Key";
    case TimesheetExposureKind::Inbetween:
        return "Inbetween";
    }
    return "Hold";
}

void fillSelectedCellExposures(Project& project,
                               const Cell& cell,
                               int startTimelineFrame,
                               int startDrawingFrameIndex,
                               int stepFrames,
                               TimesheetExposureKind kind)
{
    if (cell.id.empty()) {
        return;
    }

    const int totalFrames = safeTimelineFrameCount(project);
    const int drawingFrameCount = frameCountForCell(cell);
    startTimelineFrame = std::clamp(startTimelineFrame, 0, totalFrames - 1);
    startDrawingFrameIndex = std::clamp(startDrawingFrameIndex, 0, drawingFrameCount - 1);
    stepFrames = std::max(1, stepFrames);

    for (int timelineFrame = startTimelineFrame; timelineFrame < totalFrames; ++timelineFrame) {
        TimesheetCellExposure* exposure = ensureTimesheetExposure(project, timelineFrame, cell.id);
        if (exposure == nullptr) {
            continue;
        }
        const int offset = timelineFrame - startTimelineFrame;
        const int drawingFrameIndex = std::clamp(startDrawingFrameIndex + (offset / stepFrames),
                                                 0,
                                                 drawingFrameCount - 1);
        exposure->drawingFrameIndex = drawingFrameIndex;
        exposure->kind = kind;
    }
}

bool moveCell(Project& project, int fromIndex, int toIndex, CellPanelResult& result)
{
    const int cellCount = static_cast<int>(project.cells.size());
    if (cellCount <= 1) {
        return false;
    }

    fromIndex = std::clamp(fromIndex, 0, cellCount - 1);
    toIndex = std::clamp(toIndex, 0, cellCount - 1);
    if (fromIndex == toIndex) {
        return false;
    }

    std::swap(project.cells[static_cast<std::size_t>(fromIndex)],
              project.cells[static_cast<std::size_t>(toIndex)]);

    if (gSoloCellIndex == fromIndex) {
        gSoloCellIndex = toIndex;
    } else if (gSoloCellIndex == toIndex) {
        gSoloCellIndex = fromIndex;
    }

    rebuildCellOrderAndZ(project);

    result.selectedCellIndex = toIndex;
    result.selectionChanged = true;
    result.displayChanged = true;
    result.projectStructureChanged = true;
    return true;
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

bool duplicateCell(Project& project, int sourceIndex, CellPanelResult& result)
{
    const int cellCount = static_cast<int>(project.cells.size());
    if (cellCount <= 0) {
        return false;
    }

    sourceIndex = std::clamp(sourceIndex, 0, cellCount - 1);
    const int insertIndex = sourceIndex + 1;

    Cell duplicate = project.cells[static_cast<std::size_t>(sourceIndex)];
    duplicate.id = makeUniqueCellId(project);
    duplicate.name = makeDuplicateName(project.cells[static_cast<std::size_t>(sourceIndex)], cellCount + 1);
    duplicate.visible = true;
    duplicate.zOrder = insertIndex;

    reassignLayerIdsForCell(duplicate, insertIndex);

    const std::string duplicateCellId = duplicate.id;
    project.cells.insert(project.cells.begin() + insertIndex, std::move(duplicate));

    project.timesheet.ensureFrameCount(safeTimelineFrameCount(project));
    project.timesheet.ensureCell(duplicateCellId, 0);

    ensureCellScopedLayerIds(project);
    rebuildCellOrderAndZ(project);

    if (gSoloCellIndex >= insertIndex) {
        ++gSoloCellIndex;
    }
    if (gDisplayMode == CellDisplayMode::SoloSelected) {
        gSoloCellIndex = insertIndex;
    }

    result.selectedCellIndex = insertIndex;
    result.selectionChanged = true;
    result.displayChanged = true;
    result.projectStructureChanged = true;
    return true;
}

bool deleteCell(Project& project, int deleteIndex, CellPanelResult& result, int& editingIndex, std::string& editingCellId)
{
    const int cellCount = static_cast<int>(project.cells.size());
    if (cellCount <= 1) {
        return false;
    }

    deleteIndex = std::clamp(deleteIndex, 0, cellCount - 1);
    const std::string deletedCellId = project.cells[static_cast<std::size_t>(deleteIndex)].id;
    project.cells.erase(project.cells.begin() + deleteIndex);
    removeTimesheetCell(project, deletedCellId);

    if (editingIndex == deleteIndex) {
        editingIndex = -1;
        editingCellId.clear();
    } else if (editingIndex > deleteIndex) {
        --editingIndex;
    }

    if (gSoloCellIndex == deleteIndex) {
        gSoloCellIndex = -1;
        if (gDisplayMode == CellDisplayMode::SoloSelected) {
            gDisplayMode = CellDisplayMode::VisibleCells;
        }
    } else if (gSoloCellIndex > deleteIndex) {
        --gSoloCellIndex;
    }

    rebuildCellOrderAndZ(project);

    const int nextCount = static_cast<int>(project.cells.size());
    result.selectedCellIndex = std::clamp(deleteIndex, 0, std::max(0, nextCount - 1));
    result.selectionChanged = true;
    result.displayChanged = true;
    result.projectStructureChanged = true;
    return true;
}

Cell makeNewCell(Project& project, const char* requestedName, int categoryIndex)
{
    Cell cell;
    const int displayNumber = static_cast<int>(project.cells.size()) + 1;

    cell.id = makeUniqueCellId(project);

    const std::string name = requestedName != nullptr ? std::string(requestedName) : std::string{};
    cell.name = name.empty() ? ("Cell " + std::to_string(displayNumber)) : name;

    const int safeCategoryIndex = std::clamp(categoryIndex, 0, static_cast<int>(kCategoryOptions.size()) - 1);
    cell.category = kCategoryOptions[static_cast<std::size_t>(safeCategoryIndex)].value;

    cell.widthPx = std::max(1, project.canvas.width);
    cell.heightPx = std::max(1, project.canvas.height);
    cell.zOrder = static_cast<int>(project.cells.size());
    cell.visible = true;
    cell.opacity = 1.0f;
    cell.placement = CellPlacement{};
    cell.motionKeys.clear();
    cell.frames.clear();

    Frame frame;
    frame.name = "F001";
    frame.durationFrames = 1;
    frame.layers.clear();

    Layer layer = Layer::createDefault(1);
    layer.layerId = expectedLayerIdForCellFrame(cell, displayNumber - 1, 0, 0);
    layer.name = "Line";
    layer.visible = true;
    layer.opacity = 1.0f;
    layer.blendMode = "normal";
    layer.type = LayerType::Normal;
    layer.strokes.clear();
    layer.revisionCounter = 0U;

    frame.layers.push_back(std::move(layer));
    cell.frames.push_back(std::move(frame));

    return cell;
}

void drawDisplayModeControls(int activeCellIndex, int cellCount, CellPanelResult& result)
{
    ImGui::TextUnformatted("Display");

    int mode = 0;
    if (gDisplayMode == CellDisplayMode::VisibleCells) {
        mode = 1;
    } else if (gDisplayMode == CellDisplayMode::SoloSelected) {
        mode = 2;
    }

    if (ImGui::RadioButton("Active", mode == 0)) {
        gDisplayMode = CellDisplayMode::ActiveOnly;
        gSoloCellIndex = -1;
        result.displayChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Visible", mode == 1)) {
        gDisplayMode = CellDisplayMode::VisibleCells;
        gSoloCellIndex = -1;
        result.displayChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Solo", mode == 2)) {
        if (gDisplayMode == CellDisplayMode::SoloSelected) {
            gDisplayMode = CellDisplayMode::VisibleCells;
            gSoloCellIndex = -1;
        } else {
            gDisplayMode = CellDisplayMode::SoloSelected;
            gSoloCellIndex = std::clamp(gSoloCellIndex >= 0 ? gSoloCellIndex : activeCellIndex,
                                        0,
                                        std::max(0, cellCount - 1));
        }
        result.displayChanged = true;
    }

    if (gDisplayMode == CellDisplayMode::SoloSelected) {
        if (ImGui::Button("Clear Solo", ImVec2(-1.0f, 0.0f))) {
            gDisplayMode = CellDisplayMode::VisibleCells;
            gSoloCellIndex = -1;
            result.displayChanged = true;
        }
    }

    if (cellCount <= 0) {
        gSoloCellIndex = -1;
        if (gDisplayMode == CellDisplayMode::SoloSelected) {
            gDisplayMode = CellDisplayMode::VisibleCells;
            result.displayChanged = true;
        }
    } else if (gSoloCellIndex >= cellCount) {
        gSoloCellIndex = cellCount - 1;
    }
}

void copyStringToBuffer(char* buffer, std::size_t bufferSize, const std::string& value)
{
    if (buffer == nullptr || bufferSize == 0) {
        return;
    }
    std::snprintf(buffer, bufferSize, "%s", value.c_str());
}

void beginEditCell(int index, const Cell& cell, int& editingIndex, std::string& editingCellId, char* nameBuffer, std::size_t nameBufferSize, int& categoryIndex)
{
    editingIndex = index;
    editingCellId = cell.id;
    copyStringToBuffer(nameBuffer, nameBufferSize, cell.name.empty() ? cell.id : cell.name);
    categoryIndex = categoryIndexFromValue(cell.category);
}

void cancelEditCell(int& editingIndex, std::string& editingCellId)
{
    editingIndex = -1;
    editingCellId.clear();
}

void drawEditCellPopup(Project& project,
                       int& editingIndex,
                       std::string& editingCellId,
                       char* nameBuffer,
                       std::size_t nameBufferSize,
                       int& categoryIndex,
                       CellPanelResult& result)
{
    if (editingIndex < 0 || editingIndex >= static_cast<int>(project.cells.size())) {
        return;
    }

    Cell& cell = project.cells[static_cast<std::size_t>(editingIndex)];
    if (cell.id != editingCellId) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(380.0f, 0.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Edit Cell", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Edit Cell");
        ImGui::Separator();
        ImGui::TextDisabled("id=%s", cell.id.c_str());

        ImGui::SetNextItemWidth(280.0f);
        ImGui::InputText("Name", nameBuffer, nameBufferSize);

        categoryIndex = std::clamp(categoryIndex, 0, static_cast<int>(kCategoryOptions.size()) - 1);
        const char* preview = kCategoryOptions[static_cast<std::size_t>(categoryIndex)].label;
        ImGui::SetNextItemWidth(280.0f);
        if (ImGui::BeginCombo("Category", preview)) {
            for (int optionIndex = 0; optionIndex < static_cast<int>(kCategoryOptions.size()); ++optionIndex) {
                const bool selected = optionIndex == categoryIndex;
                if (ImGui::Selectable(kCategoryOptions[static_cast<std::size_t>(optionIndex)].label, selected)) {
                    categoryIndex = optionIndex;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();
        if (ImGui::Button("Apply", ImVec2(120.0f, 0.0f))) {
            const std::string nextName = nameBuffer != nullptr ? std::string(nameBuffer) : std::string{};
            const int safeCategoryIndex = std::clamp(categoryIndex, 0, static_cast<int>(kCategoryOptions.size()) - 1);
            cell.name = nextName.empty() ? cell.id : nextName;
            cell.category = kCategoryOptions[static_cast<std::size_t>(safeCategoryIndex)].value;
            result.displayChanged = true;
            result.projectStructureChanged = true;
            cancelEditCell(editingIndex, editingCellId);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
            cancelEditCell(editingIndex, editingCellId);
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

bool drawDeleteCellPopup(Project& project,
                         int& pendingDeleteIndex,
                         std::string& pendingDeleteCellId,
                         int& editingIndex,
                         std::string& editingCellId,
                         CellPanelResult& result)
{
    if (pendingDeleteIndex < 0 || pendingDeleteIndex >= static_cast<int>(project.cells.size())) {
        return false;
    }
    if (pendingDeleteCellId != project.cells[static_cast<std::size_t>(pendingDeleteIndex)].id) {
        return false;
    }

    ImGui::SetNextWindowSize(ImVec2(380.0f, 0.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Delete Cell", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        const Cell& cell = project.cells[static_cast<std::size_t>(pendingDeleteIndex)];
        ImGui::TextUnformatted("Delete this cell?");
        ImGui::Separator();
        ImGui::TextDisabled("id=%s", cell.id.c_str());
        ImGui::TextWrapped("This removes the cell's frames, layers, and strokes from the project.");

        ImGui::Spacing();
        if (ImGui::Button("Delete", ImVec2(120.0f, 0.0f))) {
            const bool deleted = deleteCell(project, pendingDeleteIndex, result, editingIndex, editingCellId);
            pendingDeleteIndex = -1;
            pendingDeleteCellId.clear();
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return deleted;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
            pendingDeleteIndex = -1;
            pendingDeleteCellId.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    return false;
}

void drawAddCellPopup(Project& project, CellPanelResult& result)
{
    static char nameBuffer[128] = "";
    static int categoryIndex = 0;

    if (ImGui::Button("+ Add Cell", ImVec2(-1.0f, 0.0f))) {
        const std::string defaultName = "Cell " + std::to_string(static_cast<int>(project.cells.size()) + 1);
        std::snprintf(nameBuffer, sizeof(nameBuffer), "%s", defaultName.c_str());
        categoryIndex = 0;
        ImGui::OpenPopup("Add Cell");
    }

    ImGui::SetNextWindowSize(ImVec2(380.0f, 0.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Add Cell", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Add Cell");
        ImGui::Separator();

        ImGui::SetNextItemWidth(280.0f);
        ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer));

        categoryIndex = std::clamp(categoryIndex, 0, static_cast<int>(kCategoryOptions.size()) - 1);
        const char* preview = kCategoryOptions[static_cast<std::size_t>(categoryIndex)].label;
        ImGui::SetNextItemWidth(280.0f);
        if (ImGui::BeginCombo("Category", preview)) {
            for (int i = 0; i < static_cast<int>(kCategoryOptions.size()); ++i) {
                const bool selected = i == categoryIndex;
                if (ImGui::Selectable(kCategoryOptions[static_cast<std::size_t>(i)].label, selected)) {
                    categoryIndex = i;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();
        if (ImGui::Button("Create", ImVec2(120.0f, 0.0f))) {
            Cell cell = makeNewCell(project, nameBuffer, categoryIndex);
            const std::string newCellId = cell.id;
            project.cells.push_back(std::move(cell));
            project.timesheet.ensureFrameCount(safeTimelineFrameCount(project));
            project.timesheet.ensureCell(newCellId, 0);
            syncCellOrder(project, newCellId);
            rebuildCellOrderAndZ(project);

            result.selectedCellIndex = static_cast<int>(project.cells.size()) - 1;
            result.selectionChanged = true;
            result.displayChanged = true;
            result.projectStructureChanged = true;
            gSoloCellIndex = result.selectedCellIndex;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void drawCellList(Project& project, CellPanelResult& result)
{
    ImGui::TextDisabled("Cells: %d", static_cast<int>(project.cells.size()));
    ImGui::BeginChild("CellPanel_v18b_compact_list", ImVec2(0.0f, 150.0f), true,
                      ImGuiWindowFlags_AlwaysVerticalScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 1.0f));

    for (int index = 0; index < static_cast<int>(project.cells.size()); ++index) {
        Cell& cell = project.cells[static_cast<std::size_t>(index)];
        ImGui::PushID(index);

        const bool selected = index == result.selectedCellIndex;
        const int opacityPercent = static_cast<int>(std::clamp(cell.opacity, 0.0f, 1.0f) * 100.0f + 0.5f);
        const int frameCount = static_cast<int>(cell.frames.size());
        const int firstLayers = firstFrameLayerCount(cell);

        char suffix[160]{};
        std::snprintf(suffix,
                      sizeof(suffix),
                      "  %s %3d%% z=%d F=%d L=%d",
                      cell.visible ? "V" : "-",
                      opacityPercent,
                      cell.zOrder,
                      frameCount,
                      firstLayers);

        std::string label = std::to_string(index + 1) + "  " + displayNameForCell(cell, index) + suffix;
        if (ImGui::Selectable(label.c_str(), selected, 0, ImVec2(0.0f, 0.0f))) {
            if (!selected) {
                result.selectedCellIndex = index;
                result.selectionChanged = true;
                if (gDisplayMode == CellDisplayMode::SoloSelected) {
                    gSoloCellIndex = index;
                    result.displayChanged = true;
                }
            }
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(displayNameForCell(cell, index).c_str());
            ImGui::Separator();
            ImGui::TextDisabled("id=%s", cell.id.c_str());
            ImGui::TextDisabled("visible=%s | opacity=%d%% | z=%d", cell.visible ? "true" : "false", opacityPercent, cell.zOrder);
            ImGui::TextDisabled("frames=%d | firstFrameLayers=%d | totalLayers=%d",
                                frameCount,
                                firstLayers,
                                layerCountForCell(cell));
            ImGui::EndTooltip();
        }

        ImGui::PopID();
    }

    ImGui::PopStyleVar(2);
    ImGui::EndChild();
}


void drawDetachedTimesheetWindow(Project& project, int activeCellIndex, CellPanelResult& result)
{
    if (!gTimesheetWindowOpen) {
        return;
    }
    if (project.cells.empty()) {
        return;
    }

    static int editTimelineFrame = 0;
    static int editCellIndex = -1;

    if (normalizeTimesheetForProject(project)) {
        markTimesheetChanged(result);
    }

    const int totalFrames = safeTimelineFrameCount(project);
    editTimelineFrame = std::clamp(editTimelineFrame, 0, totalFrames - 1);

    const int maxCellIndex = static_cast<int>(project.cells.size()) - 1;
    if (editCellIndex < 0 || editCellIndex > maxCellIndex) {
        editCellIndex = std::clamp(activeCellIndex, 0, maxCellIndex);
    }
    editCellIndex = std::clamp(editCellIndex, 0, maxCellIndex);

    ImGui::SetNextWindowSize(ImVec2(620.0f, 560.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Timesheet", &gTimesheetWindowOpen)) {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("Timesheet v1 - detached window");
    ImGui::TextDisabled("Edits timing data only. Canvas/playback hookup is the next step.");
    ImGui::Separator();

    int timelineFrameOneBased = editTimelineFrame + 1;
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::SliderInt("Timeline frame", &timelineFrameOneBased, 1, totalFrames, "T%03d")) {
        timelineFrameOneBased = std::clamp(timelineFrameOneBased, 1, totalFrames);
        editTimelineFrame = timelineFrameOneBased - 1;
    }

    const int activeClamped = std::clamp(activeCellIndex, 0, maxCellIndex);
    if (ImGui::Button("Use active cell")) {
        editCellIndex = activeClamped;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Active: %s", displayNameForCell(project.cells[static_cast<std::size_t>(activeClamped)], activeClamped).c_str());

    Cell& cell = project.cells[static_cast<std::size_t>(editCellIndex)];
    const std::string selectedCellLabel = displayNameForCell(cell, editCellIndex);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("Cell", selectedCellLabel.c_str())) {
        for (int index = 0; index < static_cast<int>(project.cells.size()); ++index) {
            const bool selected = index == editCellIndex;
            const std::string label = displayNameForCell(project.cells[static_cast<std::size_t>(index)], index);
            if (ImGui::Selectable(label.c_str(), selected)) {
                editCellIndex = index;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    Cell& editCell = project.cells[static_cast<std::size_t>(editCellIndex)];
    if (editCell.id.empty()) {
        ImGui::TextDisabled("Selected cell has no id.");
        ImGui::End();
        return;
    }

    TimesheetCellExposure* exposure = ensureTimesheetExposure(project, editTimelineFrame, editCell.id);
    if (exposure == nullptr) {
        ImGui::TextDisabled("Timesheet exposure unavailable.");
        ImGui::End();
        return;
    }

    const int drawingFrameCount = frameCountForCell(editCell);
    exposure->drawingFrameIndex = std::clamp(exposure->drawingFrameIndex, 0, drawingFrameCount - 1);

    ImGui::Separator();
    ImGui::TextUnformatted("Current exposure");

    int drawingFrameOneBased = exposure->drawingFrameIndex + 1;
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::SliderInt("Drawing frame", &drawingFrameOneBased, 1, drawingFrameCount, "F%03d")) {
        drawingFrameOneBased = std::clamp(drawingFrameOneBased, 1, drawingFrameCount);
        exposure->drawingFrameIndex = drawingFrameOneBased - 1;
        if (exposure->kind == TimesheetExposureKind::Null) {
            exposure->kind = TimesheetExposureKind::Hold;
        }
        markTimesheetChanged(result);
    }

    int kindIndex = 1;
    if (exposure->kind == TimesheetExposureKind::Null) {
        kindIndex = 0;
    } else if (exposure->kind == TimesheetExposureKind::Key) {
        kindIndex = 2;
    } else if (exposure->kind == TimesheetExposureKind::Inbetween) {
        kindIndex = 3;
    }

    constexpr const char* kKindLabels[] = {"Null", "Hold", "Key", "Inbetween"};
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::Combo("Kind", &kindIndex, kKindLabels, 4)) {
        kindIndex = std::clamp(kindIndex, 0, 3);
        switch (kindIndex) {
        case 0:
            exposure->kind = TimesheetExposureKind::Null;
            break;
        case 2:
            exposure->kind = TimesheetExposureKind::Key;
            break;
        case 3:
            exposure->kind = TimesheetExposureKind::Inbetween;
            break;
        default:
            exposure->kind = TimesheetExposureKind::Hold;
            break;
        }
        markTimesheetChanged(result);
    }

    const float fullWidth = ImGui::GetContentRegionAvail().x;
    const float gap = ImGui::GetStyle().ItemSpacing.x;
    const float quarter = std::max(86.0f, (fullWidth - gap * 3.0f) / 4.0f);

    if (ImGui::Button("Null", ImVec2(quarter, 0.0f))) {
        exposure->kind = TimesheetExposureKind::Null;
        markTimesheetChanged(result);
    }
    ImGui::SameLine();
    if (ImGui::Button("Hold", ImVec2(quarter, 0.0f))) {
        exposure->kind = TimesheetExposureKind::Hold;
        markTimesheetChanged(result);
    }
    ImGui::SameLine();
    if (ImGui::Button("Key", ImVec2(quarter, 0.0f))) {
        exposure->kind = TimesheetExposureKind::Key;
        markTimesheetChanged(result);
    }
    ImGui::SameLine();
    if (ImGui::Button("Inbetween", ImVec2(quarter, 0.0f))) {
        exposure->kind = TimesheetExposureKind::Inbetween;
        markTimesheetChanged(result);
    }

    const float half = std::max(120.0f, (fullWidth - gap) / 2.0f);
    if (ImGui::Button("Expose on ones to end", ImVec2(half, 0.0f))) {
        fillSelectedCellExposures(project,
                                  editCell,
                                  editTimelineFrame,
                                  exposure->drawingFrameIndex,
                                  1,
                                  TimesheetExposureKind::Hold);
        markTimesheetChanged(result);
    }
    ImGui::SameLine();
    if (ImGui::Button("Expose on twos to end", ImVec2(half, 0.0f))) {
        fillSelectedCellExposures(project,
                                  editCell,
                                  editTimelineFrame,
                                  exposure->drawingFrameIndex,
                                  2,
                                  TimesheetExposureKind::Hold);
        markTimesheetChanged(result);
    }

    if (exposure->kind == TimesheetExposureKind::Null) {
        ImGui::TextDisabled("T%03d: %s -> Null", editTimelineFrame + 1, editCell.id.c_str());
    } else {
        ImGui::TextDisabled("T%03d: %s -> F%03d (%s)",
                            editTimelineFrame + 1,
                            editCell.id.c_str(),
                            exposure->drawingFrameIndex + 1,
                            exposureKindLabel(exposure->kind));
    }

    ImGui::Separator();
    ImGui::TextUnformatted("All cells at this timeline frame");
    if (ImGui::BeginTable("TimesheetFrameOverview",
                          4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY,
                          ImVec2(0.0f, 190.0f))) {
        ImGui::TableSetupColumn("Cell");
        ImGui::TableSetupColumn("Kind");
        ImGui::TableSetupColumn("Drawing");
        ImGui::TableSetupColumn("Opacity");
        ImGui::TableHeadersRow();

        for (int index = 0; index < static_cast<int>(project.cells.size()); ++index) {
            Cell& rowCell = project.cells[static_cast<std::size_t>(index)];
            TimesheetCellExposure* rowExposure = rowCell.id.empty() ? nullptr : ensureTimesheetExposure(project, editTimelineFrame, rowCell.id);
            if (rowExposure == nullptr) {
                continue;
            }
            rowExposure->drawingFrameIndex = std::clamp(rowExposure->drawingFrameIndex, 0, frameCountForCell(rowCell) - 1);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushID(index);
            const bool selected = index == editCellIndex;
            if (ImGui::Selectable(displayNameForCell(rowCell, index).c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
                editCellIndex = index;
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(exposureKindLabel(rowExposure->kind));
            ImGui::TableSetColumnIndex(2);
            if (rowExposure->kind == TimesheetExposureKind::Null) {
                ImGui::TextDisabled("-");
            } else {
                ImGui::Text("F%03d", rowExposure->drawingFrameIndex + 1);
            }
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%d%%", static_cast<int>(std::clamp(rowCell.opacity, 0.0f, 1.0f) * 100.0f + 0.5f));
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

void drawSelectedCellControls(Project& project,
                              CellPanelResult& result,
                              int& editingCellIndex,
                              std::string& editingCellId,
                              char* editNameBuffer,
                              std::size_t editNameBufferSize,
                              int& editCategoryIndex,
                              int& pendingDeleteIndex,
                              std::string& pendingDeleteCellId)
{
    if (project.cells.empty()) {
        return;
    }

    result.selectedCellIndex = std::clamp(result.selectedCellIndex, 0, static_cast<int>(project.cells.size()) - 1);
    Cell& cell = project.cells[static_cast<std::size_t>(result.selectedCellIndex)];

    const std::string selectedName = displayNameForCell(cell, result.selectedCellIndex);
    ImGui::TextDisabled("Selected: %s", selectedName.c_str());
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(selectedName.c_str());
        ImGui::Separator();
        ImGui::TextDisabled("id=%s", cell.id.c_str());
        ImGui::TextDisabled("z=%d | frames=%d | layers=%d",
                            cell.zOrder,
                            static_cast<int>(cell.frames.size()),
                            layerCountForCell(cell));
        ImGui::EndTooltip();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));

    bool visible = cell.visible;
    if (ImGui::Checkbox("V", &visible)) {
        cell.visible = visible;
        result.displayChanged = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Visible");
    }

    ImGui::SameLine();
    ImGui::TextUnformatted("Op");
    ImGui::SameLine();
    int opacityPercent = static_cast<int>(std::clamp(cell.opacity, 0.0f, 1.0f) * 100.0f + 0.5f);
    const float opacityWidth = std::max(74.0f, ImGui::GetContentRegionAvail().x - 62.0f);
    ImGui::SetNextItemWidth(opacityWidth);
    if (ImGui::SliderInt("##CellOpacityPercentCompact", &opacityPercent, 0, 100, "%d%%")) {
        opacityPercent = std::clamp(opacityPercent, 0, 100);
        cell.opacity = static_cast<float>(opacityPercent) / 100.0f;
        result.displayChanged = true;
        result.projectStructureChanged = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Opacity percent. Stored as 0.00-1.00 float.");
    }

    const bool solo = gDisplayMode == CellDisplayMode::SoloSelected && gSoloCellIndex == result.selectedCellIndex;
    const int index = result.selectedCellIndex;
    const int cellCount = static_cast<int>(project.cells.size());

    const float fullWidth = ImGui::GetContentRegionAvail().x;
    const float gap = ImGui::GetStyle().ItemSpacing.x;
    const float third = std::max(46.0f, (fullWidth - gap * 2.0f) / 3.0f);

    if (ImGui::Button(solo ? "Solo*" : "Solo", ImVec2(third, 0.0f))) {
        if (solo) {
            gSoloCellIndex = -1;
            gDisplayMode = CellDisplayMode::VisibleCells;
        } else {
            gSoloCellIndex = result.selectedCellIndex;
            gDisplayMode = CellDisplayMode::SoloSelected;
        }
        result.displayChanged = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(solo ? "Cancel solo display" : "Show only this cell");
    }

    ImGui::SameLine();
    const bool canMoveBack = index > 0;
    if (!canMoveBack) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Back", ImVec2(third, 0.0f))) {
        moveCell(project, index, index - 1, result);
    }
    if (!canMoveBack) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    const bool canMoveFront = index + 1 < cellCount;
    if (!canMoveFront) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Front", ImVec2(third, 0.0f))) {
        moveCell(project, index, index + 1, result);
    }
    if (!canMoveFront) {
        ImGui::EndDisabled();
    }

    const float half = std::max(70.0f, (ImGui::GetContentRegionAvail().x - gap) / 2.0f);
    if (ImGui::Button("Dup", ImVec2(half, 0.0f))) {
        duplicateCell(project, result.selectedCellIndex, result);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Duplicate selected cell");
    }

    ImGui::SameLine();
    if (ImGui::Button("Edit", ImVec2(half, 0.0f))) {
        beginEditCell(result.selectedCellIndex,
                      project.cells[static_cast<std::size_t>(result.selectedCellIndex)],
                      editingCellIndex,
                      editingCellId,
                      editNameBuffer,
                      editNameBufferSize,
                      editCategoryIndex);
        ImGui::OpenPopup("Edit Cell");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Edit name and category");
    }

    const bool canDelete = cellCount > 1;
    if (!canDelete) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Delete", ImVec2(-1.0f, 0.0f))) {
        pendingDeleteIndex = result.selectedCellIndex;
        pendingDeleteCellId = project.cells[static_cast<std::size_t>(result.selectedCellIndex)].id;
        ImGui::OpenPopup("Delete Cell");
    }
    if (!canDelete) {
        ImGui::EndDisabled();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(canDelete ? "Delete selected cell" : "The last cell cannot be deleted");
    }

    ImGui::PopStyleVar(2);
}

} // namespace

CellDisplayMode currentCellDisplayMode()
{
    return gDisplayMode;
}

int currentSoloCellIndex()
{
    return gSoloCellIndex;
}

CellPanelResult drawCellPanel(Project& project, int activeCellIndex)
{
    CellPanelResult result{};
    result.selectedCellIndex = activeCellIndex;

    static int editingCellIndex = -1;
    static std::string editingCellId;
    static char editNameBuffer[128] = "";
    static int editCategoryIndex = 0;
    static int pendingDeleteIndex = -1;
    static std::string pendingDeleteCellId;

    if (ensureCellScopedLayerIds(project)) {
        result.displayChanged = true;
        result.projectStructureChanged = true;
    }
    if (rebuildCellOrderAndZ(project)) {
        result.displayChanged = true;
        result.projectStructureChanged = true;
    }

    ImGui::TextUnformatted("CellPanel v1.9e");
    drawAddCellPopup(project, result);
    ImGui::Separator();

    drawDisplayModeControls(result.selectedCellIndex,
                            static_cast<int>(project.cells.size()),
                            result);
    ImGui::Separator();

    if (project.cells.empty()) {
        ImGui::TextDisabled("No cells. Use + Add Cell.");
        result.selectedCellIndex = 0;
        return result;
    }

    const int maxIndex = static_cast<int>(project.cells.size()) - 1;
    result.selectedCellIndex = std::clamp(result.selectedCellIndex, 0, maxIndex);

    drawCellList(project, result);
    ImGui::Separator();

    ImGui::TextDisabled("Timesheet is now a separate window.");
    if (ImGui::SmallButton("Open Timesheet Window")) {
        gTimesheetWindowOpen = true;
    }
    ImGui::Separator();

    drawSelectedCellControls(project,
                             result,
                             editingCellIndex,
                             editingCellId,
                             editNameBuffer,
                             sizeof(editNameBuffer),
                             editCategoryIndex,
                             pendingDeleteIndex,
                             pendingDeleteCellId);

    drawEditCellPopup(project,
                      editingCellIndex,
                      editingCellId,
                      editNameBuffer,
                      sizeof(editNameBuffer),
                      editCategoryIndex,
                      result);

    if (drawDeleteCellPopup(project,
                            pendingDeleteIndex,
                            pendingDeleteCellId,
                            editingCellIndex,
                            editingCellId,
                            result)) {
        // A delete can change the selected index and the number of cells. Clamp once more for safety.
        if (!project.cells.empty()) {
            result.selectedCellIndex = std::clamp(result.selectedCellIndex,
                                                  0,
                                                  static_cast<int>(project.cells.size()) - 1);
        } else {
            result.selectedCellIndex = 0;
        }
    }

    drawDetachedTimesheetWindow(project, result.selectedCellIndex, result);

    return result;
}

} // namespace perapera::ui
