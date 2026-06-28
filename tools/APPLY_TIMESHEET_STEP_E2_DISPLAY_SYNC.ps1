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

function Replace-Required([string]$Source, [string]$Old, [string]$New, [string]$ErrorMessage) {
    if ($Source.Contains($Old)) {
        return $Source.Replace($Old, $New)
    }
    throw $ErrorMessage
}

# TimesheetResolver を描画モードから参照する。
if ($text -notmatch [regex]::Escape('#include "core/TimesheetResolver.h"')) {
    if ($text -match [regex]::Escape('#include "ui/App.h"')) {
        $text = $text.Replace('#include "ui/App.h"', '#include "ui/App.h" + "`r`n" + '#include "core/TimesheetResolver.h"')
    } else {
        throw '#include "ui/App.h" が見つからないため、TimesheetResolver include を追加できません。'
    }
}

# CellPanel に現在のタイムラインFを渡す。
$text = $text.Replace(
    'const ui::CellPanelResult cellPanelResult = ui::drawCellPanel(project_, activeCellIndex_);',
    'const ui::CellPanelResult cellPanelResult = ui::drawCellPanel(project_, activeCellIndex_, activeFrameIndex_);'
)

# タイムシートウィンドウ側でTを変えた場合、下のタイムライン/キャンバス表示も同じTへ移動する。
if ($text -notmatch [regex]::Escape('cellPanelResult.timelineFrameChanged')) {
    $oneLineNeedle = '}; if (cellPanelResult.projectStructureChanged) {'
    $oneLineInsert = '}; if (cellPanelResult.timelineFrameChanged) { activeFrameIndex_ = cellPanelResult.selectedTimelineFrame; clampSelection(); resetPreviewReadiness(); canvasRenderer_.markAllDirty(); lastMessage_ = u8c(u8"タイムシート表示Fを変更しました"); } if (cellPanelResult.projectStructureChanged) {'
    $multiNeedle = @'
    };
    if (cellPanelResult.projectStructureChanged) {
'@
    $multiInsert = @'
    };
    if (cellPanelResult.timelineFrameChanged) {
        activeFrameIndex_ = cellPanelResult.selectedTimelineFrame;
        clampSelection();
        resetPreviewReadiness();
        canvasRenderer_.markAllDirty();
        lastMessage_ = u8c(u8"タイムシート表示Fを変更しました");
    }
    if (cellPanelResult.projectStructureChanged) {
'@
    if ($text.Contains($oneLineNeedle)) {
        $text = $text.Replace($oneLineNeedle, $oneLineInsert)
    } elseif ($text.Contains($multiNeedle)) {
        $text = $text.Replace($multiNeedle, $multiInsert)
    } else {
        throw 'drawRightSidebar の projectStructureChanged 分岐位置が見つからないため、timelineFrameChanged 反映を追加できません。'
    }
}

# タイムシート露出変更でキャンバスをdirtyにする。
if ($text -notmatch [regex]::Escape('cellPanelResult.timesheetChanged')) {
    $oneLineOld = '} else if (cellPanelResult.displayChanged) { canvasRenderer_.markAllDirty(); lastMessage_ = "cell display changed"; } ImGui::Separator();'
    $oneLineNew = '} else if (cellPanelResult.displayChanged) { canvasRenderer_.markAllDirty(); lastMessage_ = "cell display changed"; } else if (cellPanelResult.timesheetChanged) { canvasRenderer_.markAllDirty(); lastMessage_ = u8c(u8"タイムシートを変更しました"); } ImGui::Separator();'
    $multiOld = @'
    } else if (cellPanelResult.displayChanged) {

        canvasRenderer_.markAllDirty();

        lastMessage_ = "cell display changed";

    }
'@
    $multiNew = @'
    } else if (cellPanelResult.displayChanged) {

        canvasRenderer_.markAllDirty();

        lastMessage_ = "cell display changed";

    } else if (cellPanelResult.timesheetChanged) {

        canvasRenderer_.markAllDirty();

        lastMessage_ = u8c(u8"タイムシートを変更しました");

    }
'@
    if ($text.Contains($oneLineOld)) {
        $text = $text.Replace($oneLineOld, $oneLineNew)
    } elseif ($text.Contains($multiOld)) {
        $text = $text.Replace($multiOld, $multiNew)
    } else {
        throw 'drawRightSidebar の displayChanged 分岐が見つからないため、timesheetChanged 反映を追加できません。'
    }
}

# drawCanvasArea のセル描画部分をTimesheetResolver経由に置換する。
# ここでは既存のStep E適用済みブロックも上書きし、Null露出は必ず描画しない。
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

    // Timesheet Step E2:
    // activeFrameIndex_ は「タイムラインF」として扱う。
    // 各セルの実際の作画Fは TimesheetResolver で解決する。
    // kind == Null の露出は resolved.renderable == false になるため、ここで必ず描画しない。
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
        if (!resolved.renderable || resolved.cell == nullptr || resolved.frame == nullptr) {
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

$text = $text.Substring(0, $start) + $replacement + $text.Substring($end)

if ($text -eq $original) {
    Write-Host "Timesheet Step E2: 変更はありません。すでに適用済みの可能性があります。" -ForegroundColor Yellow
    exit 0
}

$backup = $appDrawingMode + ".before_timesheet_step_e2_display_sync.bak"
[System.IO.File]::WriteAllText($backup, $original, [System.Text.Encoding]::UTF8)
[System.IO.File]::WriteAllText($appDrawingMode, $text, [System.Text.Encoding]::UTF8)

Write-Host "Timesheet Step E2 表示F同期と空セル非表示を AppDrawingMode.cpp に適用しました。" -ForegroundColor Green
Write-Host "Backup: $backup" -ForegroundColor DarkGray
