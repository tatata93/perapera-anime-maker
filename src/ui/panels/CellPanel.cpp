// This file's role: compact Cell management UI for selecting, creating, duplicating, editing, deleting, ordering, and displaying cells. v1.8d fixes opacity percent editing while keeping compact selected-cell controls.
#include "ui/panels/CellPanel.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <utility>

#include <imgui.h>

#include "ui/panels/CellPanelModel.h"

namespace perapera::ui {
namespace {

struct CategoryOption {
    const char* label;
    const char* value;
};

constexpr std::array<CategoryOption, 6> kCategoryOptions{{
    {"Character", "character"},
    {"Background", "background"},
    {"Layout", "layout"},
    {"Book", "book"},
    {"Effect", "effect"},
    {"Reference", "reference"},
}};

CellDisplayMode gDisplayMode = CellDisplayMode::VisibleCells;
int gSoloCellIndex = -1;

int categoryIndexFromValue(const std::string& value)
{
    for (int i = 0; i < static_cast<int>(kCategoryOptions.size()); ++i) {
        if (value == kCategoryOptions[static_cast<std::size_t>(i)].value) {
            return i;
        }
    }
    return 0;
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

  // Phase2Pre Step T2-c presets: create common cell categories without opening the generic Add Cell dialog.
  auto createPresetCell = [&](const char* defaultNamePrefix, const char* categoryValue) {
    const int optionIndex = categoryIndexFromValue(categoryValue);
    const std::string defaultName = std::string(defaultNamePrefix) + " " + std::to_string(static_cast<int>(project.cells.size()) + 1);
    Cell cell = makeNewCell(project, defaultName.c_str(), optionIndex);
    const std::string newCellId = cell.id;
    project.cells.push_back(std::move(cell));
    syncCellOrder(project, newCellId);
    rebuildCellOrderAndZ(project);
    result.selectedCellIndex = static_cast<int>(project.cells.size()) - 1;
    result.selectionChanged = true;
    result.displayChanged = true;
    result.projectStructureChanged = true;
    gSoloCellIndex = result.selectedCellIndex;
  };

  auto presetButton = [&](const char* buttonLabel, const char* defaultNamePrefix, const char* categoryValue, const ImVec2& size) {
    if (ImGui::Button(buttonLabel, size)) {
      createPresetCell(defaultNamePrefix, categoryValue);
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Create %s cell", categoryValue);
    }
  };

  ImGui::TextDisabled("Preset Cells");
  const float presetGap = ImGui::GetStyle().ItemSpacing.x;
  const float presetWidth = ImGui::GetContentRegionAvail().x;
  const float presetThird = std::max(72.0f, (presetWidth - presetGap * 2.0f) / 3.0f);
  presetButton("+ Char", "Character", "character", ImVec2(presetThird, 0.0f));
  ImGui::SameLine();
  presetButton("+ BG", "Background", "background", ImVec2(presetThird, 0.0f));
  ImGui::SameLine();
  presetButton("+ Layout", "Layout", "layout", ImVec2(presetThird, 0.0f));
  presetButton("+ BOOK", "BOOK", "book", ImVec2(presetThird, 0.0f));
  ImGui::SameLine();
  presetButton("+ FX", "Effect", "effect", ImVec2(presetThird, 0.0f));
  ImGui::SameLine();
  presetButton("+ Ref", "Reference", "reference", ImVec2(presetThird, 0.0f));
  ImGui::Separator();

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
        ImGui::SetTooltip("Edit cell name and final_spec_v6 category");
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

    ImGui::TextUnformatted("CellPanel Phase2-pre T2");
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

