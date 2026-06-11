# ぺらぺらアニメ作り機

ぺらぺらアニメ作り機は、C++で開発する手描き2Dアニメ制作ソフトです。

最終表現は手描き2Dアニメです。  
3D機能は、完成映像を作るためではなく、作画補助、パース補助、カメラ画角確認、動きのガイドとして使います。  
このプロジェクトはCGアニメ制作ソフトを目指しません。

## Technical Names

```text
Names
├── Display name
│   └── ぺらぺらアニメ作り機
├── Repository name
│   └── perapera-anime-maker
├── CMake project name
│   └── PeraperaAnimeMaker
└── Executable name
    └── perapera_anime_maker
```

## Current Status

```text
Status
├── Step 0
│   ├── C++20 project structure
│   ├── CMake build files
│   ├── documentation files
│   ├── license policy
│   └── work log
├── Phase 1
│   ├── SDL3 window
│   ├── Dear ImGui menu
│   └── minimal editor status panel
└── Drawing features are not implemented yet
```

## Build

```bash
cmake --preset default
cmake --build --preset default
```

Run on Windows:

```powershell
.\build\bin\perapera_anime_maker.exe
```

Run on macOS/Linux:

```bash
./build/bin/perapera_anime_maker
```

## Project Policy

```text
Policy
├── Prefer safe permissive licenses
│   ├── MIT
│   ├── BSD
│   ├── Apache-2.0
│   ├── zlib
│   └── Public Domain
├── Avoid GPL-only dependencies
├── Avoid LGPL dependencies unless approved
├── Do not use Qt in the initial phase
├── Do not statically link FFmpeg
└── Keep WORK_LOG.md updated
```