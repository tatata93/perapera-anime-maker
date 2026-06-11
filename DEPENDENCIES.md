# DEPENDENCIES

This file records all external libraries and tools used by the project.

## Current Dependencies

| Name | Purpose | License | Link | Notes |
|---|---|---|---|---|
| C++ Standard Library | Basic C++ runtime features | Implementation dependent | Compiler provided | Used by the application |
| CMake | Build configuration | BSD-3-Clause | https://cmake.org/ | Build tool, not linked into the app |
| Git | Version control | GPL-2.0 | https://git-scm.com/ | Development tool, not linked into the app |
| Git LFS | Large file management | MIT | https://git-lfs.com/ | Development tool, not linked into the app |
| SDL3 | Window creation, input, and renderer | zlib | https://github.com/libsdl-org/SDL | Added in Phase 1 |
| Dear ImGui | Immediate mode editor UI | MIT | https://github.com/ocornut/imgui | Added in Phase 1 |

## Why SDL3 is used

```text
SDL3
├── 何をするためのものか
│   ├── ウィンドウを作る
│   ├── マウス・キーボード入力を受け取る
│   └── SDL_Rendererを作る
├── なぜ必要か
│   └── OSごとのウィンドウ処理を直接書かずに済むため
├── 代替手段
│   ├── GLFW
│   ├── raw Win32 API
│   └── Qt
├── ライセンス
│   └── zlib
└── このプロジェクトでの使い方
    └── Phase 1では最小ウィンドウ表示に使う