# Phase 2 Step 2-k App IO Inventory

Generated from local repository.

## Branch

`	ext
main
`"
# Phase 2 Step 2-k App IO Inventory  Generated from local repository.  ## Branch  `	ext main += "

# Phase 2 Step 2-k App IO Inventory  Generated from local repository.  ## Branch  `	ext main += 

### src\ui\AppProjectIO.cpp

- line 18: `#include "io/ProjectLayoutSaveEntry.h"`
- line 99: `bool saveProjectNewLayoutForActiveCut(const std::filesystem::path& projectFolder,`
- line 102: `ProjectLayoutSaveEntryResult* result,`
- line 107: `return saveProjectNewLayoutMinimal(projectFolder, scene, cut, project.cells, result, error);`
- line 112: `void App::saveProject()`
- line 119: `if (!ProjectIO::save(toSave, projectFolder, &error)) {`
- line 124: `ProjectLayoutSaveEntryResult newLayoutResult;`
- line 125: `if (!saveProjectNewLayoutForActiveCut(projectFolder, toSave, workingTimesheet_, &newLayoutResult, &error)) {`
- line 132: `lastMessage_ = "project saved, but app_state failed: " + error;`
- line 142: `// 以前はここでProjectIO::load()してprojectSignature()まで走査していた。`
- line 144: `// 検証は saveLoadRoundTripCheck() の明示ボタンだけで行う。`
- line 154: `void App::loadProject()`
- line 159: `if (!ProjectIO::load(projectFolder, loaded, &error)) {`
- line 166: `std::string selectionSource = "app_state";`
- line 207: `void App::saveLoadRoundTripCheck()`
- line 216: `if (!ProjectIO::save(toSave, projectFolder, &error)) {`
- line 221: `ProjectLayoutSaveEntryResult newLayoutResult;`
- line 222: `if (!saveProjectNewLayoutForActiveCut(projectFolder, toSave, workingTimesheet_, &newLayoutResult, &error)) {`
- line 228: `lastMessage_ = "round trip app_state failed: " + error;`
- line 238: `if (!ProjectIO::load(projectFolder, loaded, &error)) {`

### src\ui\AppProjectIOSupport.cpp

- line 110: `return projectFolder / "app_state.json";`
- line 426: `*error = "failed to write app_state.json: " + appStatePath(projectFolder).string();`

### src\io\ProjectIO.cpp

- line 825: `bool ProjectIO::save(const Project& project, const fs::path& folderPath, std::string* errorMessage)`
- line 853: `bool ProjectIO::load(const fs::path& folderPath, Project& project, std::string* errorMessage)`

### src\io\ProjectLayoutSaveEntry.cpp

- line 1: `#include "io/ProjectLayoutSaveEntry.h"`
- line 5: `#include "io/ProjectLayoutPaths.h"`
- line 20: `bool saveProjectNewLayoutMinimal(const std::filesystem::path& projectRoot,`
- line 24: `ProjectLayoutSaveEntryResult* outResult,`

### src\io\ProjectLayoutLoadEntry.cpp

- line 1: `#include "io/ProjectLayoutLoadEntry.h"`
- line 13: `#include "io/ProjectLayoutPaths.h"`
- line 300: `bool loadProjectNewLayoutMinimal(`
- line 302: `const ProjectLayoutLoadOptions& options,`
- line 303: `ProjectLayoutLoadResult* outResult,`
- line 313: `ProjectLayoutLoadResult result;`

### src\io\ProjectLayoutReadEntry.cpp

- line 1: `#include "io/ProjectLayoutReadEntry.h"`
- line 4: `#include "io/ProjectLayoutPaths.h"`
- line 130: `ProjectLayoutReadSummary& summary,`
- line 170: `const ProjectLayoutReadOptions& options,`
- line 171: `ProjectLayoutReadSummary* outSummary,`
- line 177: `ProjectLayoutReadSummary summary;`

## Files over 800 lines

- src\io\ProjectIO.cpp : 804 lines
- src\render\CanvasRenderer.cpp : 895 lines
- src\ui\panels\CellPanel.cpp : 824 lines
- src\ui\AppDrawingMode.cpp : 1784 lines
