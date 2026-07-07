# Claude Handoff - perapera-anime-maker

This file is the temporary handoff document for another coding assistant.
It must stay visible at the repository root on GitHub.

Repository:

- GitHub: `https://github.com/tatata93/perapera-anime-maker`
- Branch to use: `codex/fillstroke-crop-cache`
- Main executable that must exist after build: `build/bin/perapera_anime_maker.exe`

## Current work tree

```text
final_spec_v6.md
└─ Phase 2: ファイル構成改定 + セル概念の整理
   ├─ Phase 2-pre: cleanup before main migration
   │  ├─ T1: remove Scene Plate / scene panel from main flow       done
   │  └─ T2: normal Cell category + Timesheet policy               done
   │
   ├─ Phase 2 Step 1: minimal Project -> Scene -> Cut -> Cell model
   │  ├─ 1-a: Scene / active Cut model                             done
   │  ├─ 1-b: Project.cells treated as temporary active Cut cells   done
   │  ├─ 1-c: UI status display                                    reverted / hold
   │  └─ 1-d: Cut Cell and Timesheet connection bridge              in verification
   │
   └─ Phase 2 Step 2: migrate save layout to scenes/scene_NNN/cuts/cut_NNN/cells
      ├─ 2-a: path helpers                                         done
      ├─ 2-b: minimal scene.json / cut.json save                   done
      ├─ 2-c: cell.json / frames directory save                    done
      ├─ 2-d: layer_NNN.json / stroke save                         verified
      ├─ 2-e: one top-level save entry for new layout               done
      ├─ 2-f: connect new-layout save entry to app save path        done
      └─ 2-g: inspect/read entry for new layout                     done
```

## Immediate first action

Step 2-d through Step 2-g have been verified locally.

Before adding new features, keep the tree clean enough to hand off:

- Commit or intentionally discard unrelated untracked notes.
- Do not restore Scene Plate / ScenePlateImageCache / scene panel code.
- Keep new save-layout work in the root project, not in a nested `perapera-anime-maker/` folder.

## Current design decisions

- `final_spec_v6.md` is the top-level spec.
- Backward compatibility with old project save formats is not required because this is still in development.
- New structure should be prioritized over old `ProjectIO` compatibility.
- Scene Plate / scene management panel must not return.
- Background, layout, BOOK, effect, reference are normal Cell categories, not Scene Plate objects.
- Timesheet is important and should remain physically separate, for example `timesheet.json`.
- Background / layout / BOOK cells should be timed through the Timesheet like normal cells.
- UI work should be conservative until the data model and save layout are stable.
- The failed Step 1-c UI status panel is on hold.

## Current Cell categories

Use these exact category values:

```text
character
background
layout
book
effect
reference
```

Do not reintroduce `camera_guide` or `other` as main category values unless the user explicitly asks.

## File-size and cleanup rules

The user explicitly requested:

- If a file would exceed 800 lines, split it or lighten it by default.
- Do not add unnecessary files or functions.
- If unnecessary files/functions are found, remove or lighten them after considering feature impact.
- Avoid growing large UI files such as `AppDrawingMode.cpp`.
- Prefer small helper files and selftests.
- PowerShell apply scripts should be ASCII-only. Do not use Japanese text as string anchors in PowerShell scripts.

## Build rules

Always check that the executable exists. Selftests passing without `perapera_anime_maker.exe` is not enough.

Use VS Code integrated terminal / PowerShell.

Full clean build command:

```powershell
cd C:\Users\tak01\github\perapera-anime-maker

Get-Process perapera_anime_maker -ErrorAction SilentlyContinue | Stop-Process -Force
Get-Process msbuild -ErrorAction SilentlyContinue | Stop-Process -Force
Get-Process cl -ErrorAction SilentlyContinue | Stop-Process -Force
Get-Process link -ErrorAction SilentlyContinue | Stop-Process -Force
Get-Process cmake -ErrorAction SilentlyContinue | Stop-Process -Force

if (Test-Path .\build) {
    Remove-Item .\build -Recurse -Force
}

cmake -S . -B .\build -G "Visual Studio 18 2026"

if ($LASTEXITCODE -eq 0) {
    cmake --build .\build --config Debug --parallel 1 -- /m:1 /nodeReuse:false *> .\build_full_check.log
}

if (Test-Path .\build\bin\perapera_anime_maker.exe) {
    Write-Host "exe OK" -ForegroundColor Green
    Get-Item .\build\bin\perapera_anime_maker.exe
} else {
    Write-Host "exe NG" -ForegroundColor Red
    Get-Content .\build_full_check.log -Tail 250
}
```

## Git rules

Do not use:

```powershell
git add .
```

Use individual `git add` lines for changed files.

Before any commit:

```powershell
git status
```

After commit:

```powershell
git push origin codex/fillstroke-crop-cache
```

## Recommended next step

Proceed to:

```text
Phase 2 Step 2-h: turn the new-layout inspector into a real app load path
```

Goal:

- Use `inspectProjectNewLayoutMinimal()` as the basis for a real load path.
- Keep UI changes minimal until the new load path is stable.
- Do not reintroduce Scene Plate or old scene-panel concepts.
- Include build verification that `build/bin/perapera_anime_maker.exe` still exists.

## What not to do next

Do not jump to these yet:

- UI scene/cut selector
- shooting/compositing panel
- camera transform window
- legacy project compatibility loader
- broad rewrite of `Project.h`
- broad rewrite of `ProjectIO.cpp`
- reintroducing Scene Plate or scene panel

## Handoff summary

Claude should first confirm the local branch is clean enough to proceed, then continue from Step 2-h. Step 2-g adds `src/io/ProjectLayoutReadEntry.*` and `tools/project_layout_read_entry_selftest.cpp`.
