// This file's role: compact Cell management UI for selecting, creating, duplicating, editing, deleting, ordering, and displaying cells.
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

    project.cells.insert(project.cells.begin() + insertIndex, std::move(duplicate));

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
    project.cells.erase(project.cells.begin() + deleteIndex);

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

    ImGui::TextUnformatted("Selected Cell");
    ImGui::TextDisabled("%s", displayNameForCell(cell, result.selectedCellIndex).c_str());

    bool visible = cell.visible;
    if (ImGui::Checkbox("Visible", &visible)) {
        cell.visible = visible;
        result.displayChanged = true;
    }

    float opacity = std::clamp(cell.opacity, 0.0f, 1.0f);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f, "%.2f")) {
        cell.opacity = std::clamp(opacity, 0.0f, 1.0f);
        result.displayChanged = true;
    }

    const bool solo = gDisplayMode == CellDisplayMode::SoloSelected && gSoloCellIndex == result.selectedCellIndex;
    if (ImGui::Button(solo ? "Solo: ON" : "Solo", ImVec2(-1.0f, 0.0f))) {
        if (solo) {
            gSoloCellIndex = -1;
            gDisplayMode = CellDisplayMode::VisibleCells;
        } else {
            gSoloCellIndex = result.selectedCellIndex;
            gDisplayMode = CellDisplayMode::SoloSelected;
        }
        result.displayChanged = true;
    }

    const int index = result.selectedCellIndex;
    const int cellCount = static_cast<int>(project.cells.size());

    const bool canMoveBack = index > 0;
    const bool canMoveFront = index + 1 < cellCount;
    if (!canMoveBack) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Back", ImVec2(86.0f, 0.0f))) {
        moveCell(project, index, index - 1, result);
    }
    if (!canMoveBack) {
        ImGui::EndDisabled();
    }
    ImGui::SameLine();
    if (!canMoveFront) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Front", ImVec2(86.0f, 0.0f))) {
        moveCell(project, index, index + 1, result);
    }
    if (!canMoveFront) {
        ImGui::EndDisabled();
    }

    if (ImGui::Button("Duplicate", ImVec2(-1.0f, 0.0f))) {
        duplicateCell(project, result.selectedCellIndex, result);
    }

    if (ImGui::Button("Edit Name/Category", ImVec2(-1.0f, 0.0f))) {
        beginEditCell(result.selectedCellIndex,
                      project.cells[static_cast<std::size_t>(result.selectedCellIndex)],
                      editingCellIndex,
                      editingCellId,
                      editNameBuffer,
                      editNameBufferSize,
                      editCategoryIndex);
        ImGui::OpenPopup("Edit Cell");
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

    ImGui::TextDisabled("Tip: select a cell above, then use these controls.");
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

    ImGui::TextUnformatted("CellPanel v1.8b");
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

    return result;
}

} // namespace perapera::ui
