# Phase 2 Step 2-m ProjectIO usage audit

This report was generated from the local repository.

## ProjectIO references outside src/io/ProjectIO.*

- :11: `// - Do not change ProjectIO or the physical save format in this step.`
- :9: `// - Project.h / ProjectIO.cpp の保存形式はここでは変更しない。`
- :9: `#include "io/ProjectIO.h"`
- :825: `bool ProjectIO::save(const Project& project, const fs::path& folderPath, std::string* errorMessage)`
- :853: `bool ProjectIO::load(const fs::path& folderPath, Project& project, std::string* errorMessage)`
- :14: `class ProjectIO {`
- :4: `// Legacy ProjectIO compatibility is intentionally not handled here.`
- :5: `// without pulling in legacy ProjectIO behavior.`
- :5: `// This does not replace current ProjectIO yet.`
- :13: `#include "ui/AppProjectIOSupport.h"`
- :19: `#include "ui/AppProjectIOSupport.h"`
- :12: `#include "ui/AppProjectIOSupport.h"`
- :17: `#include "io/ProjectIO.h"`
- :19: `#include "ui/AppProjectIOSupport.h"`
- :119: `if (!ProjectIO::save(toSave, projectFolder, &error)) {`
- :142: `// 以前はここでProjectIO::load()してprojectSignature()まで走査していた。`
- :159: `if (!ProjectIO::load(projectFolder, loaded, &error)) {`
- :216: `if (!ProjectIO::save(toSave, projectFolder, &error)) {`
- :238: `if (!ProjectIO::load(projectFolder, loaded, &error)) {`
- :5: `#include "ui/AppProjectIOSupport.h"`
- :4: `// AppProjectIO.cpp の保存・読み込み補助処理を分離する。`
- :150: `src/io/ProjectIO.cpp`
- :171: `src/ui/AppProjectIO.cpp`
- :173: `src/ui/AppProjectIOSupport.cpp`

## Files over 800 lines

- src\ui\AppDrawingMode.cpp: 1784 lines
- src\render\CanvasRenderer.cpp: 895 lines
- src\ui\panels\CellPanel.cpp: 824 lines
- src\io\ProjectIO.cpp: 804 lines

## Action

ProjectIO still has references. The apply script did not delete ProjectIO.*. Remove or replace the listed references first.