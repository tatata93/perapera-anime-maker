# Timesheet Rebuild Step 7.12.1: Scene Plate UI build fix

## Purpose

Fix the Windows build failure introduced in Step 7.12 Scene Plate UI.

## Cause

`ImGui::PushID(plate.id.empty() ? plateIndex : plate.id.c_str())` mixed `int` and `const char*` in a ternary expression. MSVC cannot choose a common type for those operands, so `AppDrawingMode.cpp` failed to compile and `perapera_anime_maker.exe` was not produced.

## Fix

Use explicit branches:

- empty ScenePlate id: `ImGui::PushID(plateIndex)`
- non-empty ScenePlate id: `ImGui::PushID(plate.id.c_str())`

This keeps stable ImGui IDs without relying on an invalid ternary expression.

## Scope

Only `src/ui/AppDrawingMode.cpp` is changed. No model, IO, CMake, or project format changes are included.
