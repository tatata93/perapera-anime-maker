// This file's role: Cell management UI for selecting cells, editing visibility/opacity, and adding new cells.
#include "ui/panels/CellPanel.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <functional>
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

Cell makeNewCell(Project& project, const char* requestedName, int categoryIndex)
{
    Cell cell = Cell::createDefault();
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

    if (cell.frames.empty()) {
        cell.frames.push_back(Frame::createDefault(1));
    }
    if (cell.frames.front().layers.empty()) {
        cell.frames.front().layers.push_back(Layer::createDefault(1));
    }

    for (int frameIndex = 0; frameIndex < static_cast<int>(cell.frames.size()); ++frameIndex) {
        Frame& frame = cell.frames[static_cast<std::size_t>(frameIndex)];
        for (int layerIndex = 0; layerIndex < static_cast<int>(frame.layers.size()); ++layerIndex) {
            frame.layers[static_cast<std::size_t>(layerIndex)].layerId =
                expectedLayerIdForCellFrame(cell, displayNumber - 1, frameIndex, layerIndex);
        }
    }

    return cell;
}

void drawAddCellSection(Project& project, CellPanelResult& result)
{
    static bool addOpen = false;
    static char nameBuffer[128] = "";
    static int categoryIndex = 0;

    if (ImGui::Button("+ Add Cell", ImVec2(-1.0f, 0.0f))) {
        addOpen = !addOpen;
        if (nameBuffer[0] == '\0') {
            const std::string defaultName = "Cell " + std::to_string(static_cast<int>(project.cells.size()) + 1);
            std::snprintf(nameBuffer, sizeof(nameBuffer), "%s", defaultName.c_str());
        }
    }

    if (!addOpen) {
        return;
    }

    ImGui::BeginChild("CellPanel_v11_add", ImVec2(0.0f, 112.0f), true);
    ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer));

    const char* preview = kCategoryOptions[static_cast<std::size_t>(std::clamp(
        categoryIndex, 0, static_cast<int>(kCategoryOptions.size()) - 1))].label;
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

    if (ImGui::Button("Create", ImVec2(110.0f, 0.0f))) {
        Cell cell = makeNewCell(project, nameBuffer, categoryIndex);
        const std::string newCellId = cell.id;
        project.cells.push_back(std::move(cell));
        syncCellOrder(project, newCellId);

        result.selectedCellIndex = static_cast<int>(project.cells.size()) - 1;
        result.selectionChanged = true;
        result.displayChanged = true;
        result.projectStructureChanged = true;

        nameBuffer[0] = '\0';
        categoryIndex = categoryIndexFromValue(project.cells.back().category);
        addOpen = false;
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f))) {
        addOpen = false;
    }
    ImGui::EndChild();
}

} // namespace

CellPanelResult drawCellPanel(Project& project, int activeCellIndex)
{
    CellPanelResult result{};
    result.selectedCellIndex = activeCellIndex;

    if (ensureCellScopedLayerIds(project)) {
        result.displayChanged = true;
        result.projectStructureChanged = true;
    }

    ImGui::TextUnformatted("CellPanel v1.1b");
    drawAddCellSection(project, result);
    ImGui::Separator();

    if (project.cells.empty()) {
        ImGui::TextDisabled("No cells. Use + Add Cell.");
        result.selectedCellIndex = 0;
        return result;
    }

    const int maxIndex = static_cast<int>(project.cells.size()) - 1;
    result.selectedCellIndex = std::clamp(result.selectedCellIndex, 0, maxIndex);

    ImGui::BeginChild("CellPanel_v11_list", ImVec2(0.0f, 210.0f), true,
                      ImGuiWindowFlags_AlwaysVerticalScrollbar);

    for (int index = 0; index < static_cast<int>(project.cells.size()); ++index) {
        Cell& cell = project.cells[static_cast<std::size_t>(index)];
        ImGui::PushID(index);

        const bool selected = index == result.selectedCellIndex;
        const std::string label = displayNameForCell(cell, index);
        if (ImGui::Selectable(label.c_str(), selected)) {
            if (!selected) {
                result.selectedCellIndex = index;
                result.selectionChanged = true;
            }
        }

        bool visible = cell.visible;
        if (ImGui::Checkbox("Visible", &visible)) {
            cell.visible = visible;
            result.displayChanged = true;
        }

        ImGui::SameLine();
        float opacity = std::clamp(cell.opacity, 0.0f, 1.0f);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f, "%.2f")) {
            cell.opacity = std::clamp(opacity, 0.0f, 1.0f);
            result.displayChanged = true;
        }

        ImGui::TextDisabled("id=%s | z=%d | frames=%d | layers=%d",
                            cell.id.c_str(),
                            cell.zOrder,
                            static_cast<int>(cell.frames.size()),
                            layerCountForCell(cell));

        ImGui::Separator();
        ImGui::PopID();
    }

    ImGui::EndChild();
    return result;
}

} // namespace perapera::ui
