param(
    [string]$RepoRoot = "C:\Users\tak01\github\perapera-anime-maker"
)

$ErrorActionPreference = "Stop"

$appDrawingMode = Join-Path $RepoRoot "src\ui\AppDrawingMode.cpp"
if (-not (Test-Path $appDrawingMode)) {
    throw "AppDrawingMode.cpp が見つかりません: $appDrawingMode"
}

$text = [System.IO.File]::ReadAllText($appDrawingMode, [System.Text.Encoding]::UTF8)
$original = $text

# Step E: TimesheetResolver を描画モードから参照する。
if ($text -notmatch [regex]::Escape('#include "core/TimesheetResolver.h"')) {
    if ($text -match [regex]::Escape('#include "ui/App.h"')) {
        $text = $text.Replace('#include "ui/App.h"', '#include "ui/App.h"' + "`r`n" + '#include "core/TimesheetResolver.h"')
    } else {
        throw '#include "ui/App.h" が見つからないため、TimesheetResolver include を追加できません。'
    }
}

# CellPanel側のタイムシート編集結果でキャンバスをdirtyにする。
$displayBranch = @'
    } else if (cellPanelResult.displayChanged) {

        canvasRenderer_.markAllDirty();

        lastMessage_ = "cell display changed";

    }
'@
$displayBranchLf = $displayBranch -replace "`r`n", "`n"
$timesheetBranch = @'
    } else if (cellPanelResult.displayChanged) {

        canvasRenderer_.markAllDirty();

        lastMessage_ = "cell display changed";

    } else if (cellPanelResult.timesheetChanged) {

        canvasRenderer_.markAllDirty();

        lastMessage_ = u8c(u8"タイムシートを変更しました");

    }
'@
$timesheetBranchLf = $timesheetBranch -replace "`r`n", "`n"

if ($text -notmatch [regex]::Escape('cellPanelResult.timesheetChanged')) {
    if ($text.Contains($displayBranch)) {
        $text = $text.Replace($displayBranch, $timesheetBranch)
    } elseif ($text.Contains($displayBranchLf)) {
        $text = $text.Replace($displayBranchLf, $timesheetBranchLf)
    } else {
        throw 'drawRightSidebar の displayChanged 分岐が見つからないため、timesheetChanged 反映を追加できません。'
    }
}

# drawCanvasArea のセル描画部分を TimesheetResolver 経由に置換する。
$startNeedle = 'const ui::CellDisplayMode cellDisplayMode = ui::currentCellDisplayMode();'
$endNeedle = 'warmPlaybackFrameCache();'
$start = $text.IndexOf($startNeedle)
if ($start -lt 0) {
    throw 'drawCanvasArea 内の cellDisplayMode 開始位置が見つかりません。'
}
$end = $text.IndexOf($endNeedle, $start)
if ($end -lt 0) {
    throw 'drawCanvasArea 内の warmPlaybackFrameCache() が見つかりません。'
}

$replacement = @'
const ui::CellDisplayMode cellDisplayMode = ui::currentCellDisplayMode();
    const int soloCellIndex = ui::currentSoloCellIndex();

    auto drawCellFrame = [&](int cellIndex, const Cell& drawCell, const Frame& drawFrame, int drawFrameIndex) {

        const bool isActiveCell = cellIndex == activeCellIndex_;

        const int drawActiveLayer = isActiveCell ? activeLayerIndex_ : -1;

        const Stroke* drawPreview = isActiveCell ? preview : nullptr;

        const float drawOpacity = std::clamp(drawCell.opacity, 0.0f, 1.0f);

        if (drawOpacity <= 0.0f) {

            return;

        }

        canvasRenderer_.draw(drawFrame,
            drawFrameIndex,

            drawActiveLayer,

            drawPreview,

            drawOpacity,

            canvasView_,

            canvasDisplayMode,

            areaMin,

            areaSize,

            drawList);

    };

    auto drawResolvedCell = [&](const ResolvedTimesheetCell& resolved) {
        if (resolved.cell == nullptr || resolved.frame == nullptr || !resolved.timesheetVisible) {
            return;
        }
        drawCellFrame(resolved.cellIndex, *resolved.cell, *resolved.frame, resolved.drawingFrameIndex);
    };

    if (cellDisplayMode == ui::CellDisplayMode::ActiveOnly || project_.cells.empty()) {

        const ResolvedTimesheetCell resolved = TimesheetResolver::resolveCell(project_, activeFrameIndex_, activeCellIndex_);
        drawResolvedCell(resolved);

    } else if (cellDisplayMode == ui::CellDisplayMode::SoloSelected) {

        const int safeSoloIndex = std::clamp(soloCellIndex >= 0 ? soloCellIndex : activeCellIndex_,
            0,
            static_cast<int>(project_.cells.size()) - 1);

        const ResolvedTimesheetCell resolved = TimesheetResolver::resolveCell(project_, activeFrameIndex_, safeSoloIndex);
        drawResolvedCell(resolved);

    } else {

        TimesheetResolveOptions resolveOptions{};
        resolveOptions.includeHiddenCells = false;
        resolveOptions.sortByZOrder = true;
        const ResolvedTimesheetFrame resolvedFrame = TimesheetResolver::resolveFrame(project_, activeFrameIndex_, resolveOptions);

        for (const ResolvedTimesheetCell& resolved : resolvedFrame.cells) {
            drawResolvedCell(resolved);
        }

    }

    
'@

# すでにStep E適用済みなら二重置換しない。
$existingBlock = $text.Substring($start, $end - $start)
if ($existingBlock -notmatch [regex]::Escape('TimesheetResolver::resolveFrame')) {
    $text = $text.Substring(0, $start) + $replacement + $text.Substring($end)
}

if ($text -eq $original) {
    Write-Host "Timesheet Step E: 変更はありません。すでに適用済みの可能性があります。" -ForegroundColor Yellow
    exit 0
}

$backup = $appDrawingMode + ".before_timesheet_step_e_display.bak"
[System.IO.File]::WriteAllText($backup, $original, [System.Text.Encoding]::UTF8)
[System.IO.File]::WriteAllText($appDrawingMode, $text, [System.Text.Encoding]::UTF8)

Write-Host "Timesheet Step E を AppDrawingMode.cpp に適用しました。" -ForegroundColor Green
Write-Host "Backup: $backup" -ForegroundColor DarkGray
