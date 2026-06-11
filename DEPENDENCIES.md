# DEPENDENCIES

This file records all external libraries and tools used by the project.

## Current Dependencies

At Step 0, no third-party libraries are linked into the application.

| Name | Purpose | License | Link | Notes |
|---|---|---|---|---|
| C++ Standard Library | Basic C++ runtime features | Implementation dependent | Compiler provided | Used by `main.cpp` |
| CMake | Build configuration | BSD-3-Clause | https://cmake.org/ | Build tool, not linked into the app |
| Git | Version control | GPL-2.0 | https://git-scm.com/ | Development tool, not linked into the app |
| Git LFS | Large file management | MIT | https://git-lfs.com/ | Development tool, not linked into the app |

## Planned Dependencies

These are planned but not yet added.

| Name | Purpose | License | Status | Notes |
|---|---|---|---|---|
| SDL3 | Window creation and input | zlib | Planned | Requires approval before adding |
| Dear ImGui | Editor UI | MIT | Planned | Requires approval before adding |
| Skia | 2D drawing engine | BSD-3-Clause | Planned | Requires approval before adding |
| GLM | Vector and matrix math | MIT | Planned | Requires approval before adding |
| nlohmann/json | JSON save files | MIT | Planned | Requires approval before adding |
| SQLite | Project database | Public Domain | Planned | Requires approval before adding |
| OpenEXR | Professional image sequence output | BSD-3-Clause | Planned | Requires approval before adding |
| OpenTimelineIO | Timeline exchange | Apache-2.0 | Planned | Requires approval before adding |
| OpenColorIO | Color management | BSD-3-Clause | Planned | Requires approval before adding |
| Box2D | Optional 2D physics | MIT | Planned | Requires approval before adding |
| FFmpeg | External video conversion | LGPL/GPL depending on build | Optional external tool | Do not link at first |