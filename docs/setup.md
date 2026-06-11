# Setup Guide

This document explains how to set up the development environment for ぺらぺらアニメ作り機.

## Required Tools

```text
Development Environment
├── Git
├── Git LFS
├── CMake
├── C++20 compiler
│   ├── Windows: Visual Studio 2022 Build Tools
│   ├── macOS: Xcode Command Line Tools
│   └── Linux: GCC or Clang
└── VSCode
```

## Build

From the repository root:

```bash
cmake --preset default
cmake --build --preset default
```

## Run

Windows:

```powershell
.\build\bin\perapera_anime_maker.exe
```

macOS/Linux:

```bash
./build/bin/perapera_anime_maker
```

## GitHub Setup

Initialize the repository:

```bash
git init
git lfs install
git add .
git commit -m "Step 0: create initial project for Perapera Anime Maker"
git branch -M main
```

If using GitHub CLI:

```bash
gh auth login
gh repo create perapera-anime-maker --private --source=. --remote=origin --push
```

## Notes

- Start with a private repository.
- Make it public only after license and dependency checks are complete.
- Keep `WORK_LOG.md` updated after each task.