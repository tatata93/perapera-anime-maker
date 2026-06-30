# Timesheet Rebuild Step 7.14.1: Scene Plate image path build fix

## Purpose
Fix the main application build failure introduced in Step 7.14.

`perapera_anime_maker.exe` was not generated because `AppDrawingMode.cpp` referenced `projectFolder` in the Scene Plate image path UI before that variable was declared in the relevant scope.

## Fixed issue

Compiler error:

```text
src/ui/AppDrawingMode.cpp(1983): error C2065: 'projectFolder': undeclared identifier
src/ui/AppDrawingMode.cpp(1984): error C2065: 'projectFolder': undeclared identifier
src/ui/AppDrawingMode.cpp(1993): error C2065: 'projectFolder': undeclared identifier
```

## Change

The Scene Plate manager block now declares the project folder once before the image path UI uses it:

```cpp
const int currentT = std::max(1, timesheetPanelState_.selectedTimelineFrame + 1);
const std::filesystem::path projectFolder = appio::absolutePath(exportState_.projectFolder);
```

The later `cut.json` save/load area reuses the same variable instead of declaring it after the image path UI.

## Verification priority

The main application executable must be generated before this step is considered successful:

```text
build/bin/perapera_anime_maker.exe
```

Selftests alone are not sufficient, because they can pass while the main app target fails.
