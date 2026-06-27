param(
    [string]$RepoRoot = "C:\Users\tak01\github\perapera-anime-maker",
    [string]$ProjectDir = ""
)

$ErrorActionPreference = "Stop"

function Write-Ok($msg) { Write-Host "[OK] $msg" -ForegroundColor Green }
function Write-WarnLine($msg) { Write-Host "[WARN] $msg" -ForegroundColor Yellow }
function Write-ErrLine($msg) { Write-Host "[ERROR] $msg" -ForegroundColor Red }
function Write-Info($msg) { Write-Host "[INFO] $msg" -ForegroundColor Cyan }

function Read-JsonFile($path) {
    if (-not (Test-Path $path)) {
        throw "Missing json file: $path"
    }
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $text = [System.Text.Encoding]::UTF8.GetString($bytes)
    return $text | ConvertFrom-Json
}

function Get-PropertyValue($obj, $name, $fallback) {
    if ($null -eq $obj) { return $fallback }
    $prop = $obj.PSObject.Properties[$name]
    if ($null -eq $prop) { return $fallback }
    return $prop.Value
}

function Add-Warning([System.Collections.Generic.List[string]]$warnings, $message) {
    $warnings.Add($message) | Out-Null
    Write-WarnLine $message
}

function Add-Error([System.Collections.Generic.List[string]]$errors, $message) {
    $errors.Add($message) | Out-Null
    Write-ErrLine $message
}

Write-Host "CellPanel v1.7 save/load audit" -ForegroundColor Cyan
Write-Host "RepoRoot: $RepoRoot"

if ([string]::IsNullOrWhiteSpace($ProjectDir)) {
    $candidateA = Join-Path $RepoRoot "my_anime_project"
    $candidateB = Join-Path $RepoRoot "project"
    if (Test-Path (Join-Path $candidateA "project.json")) {
        $ProjectDir = $candidateA
    } elseif (Test-Path (Join-Path $candidateB "project.json")) {
        $ProjectDir = $candidateB
    } else {
        Write-WarnLine "ProjectDir was not specified and no default project.json was found."
        Write-Host "Run again with:"
        Write-Host "  powershell -ExecutionPolicy Bypass -File .\tools\VERIFY_CELL_SAVE_LOAD.ps1 -RepoRoot `"$RepoRoot`" -ProjectDir `"<saved project folder>`""
        return
    }
}

Write-Host "ProjectDir: $ProjectDir"

$warnings = [System.Collections.Generic.List[string]]::new()
$errors = [System.Collections.Generic.List[string]]::new()

try {
    $projectJsonPath = Join-Path $ProjectDir "project.json"
    $cellsRoot = Join-Path $ProjectDir "cells"

    $project = Read-JsonFile $projectJsonPath
    if (-not (Test-Path $cellsRoot)) {
        Add-Error $errors "Missing cells folder: $cellsRoot"
    } else {
        Write-Ok "cells folder exists"
    }

    $cellOrder = @()
    $rawCellOrder = Get-PropertyValue $project "cellOrder" @()
    if ($null -ne $rawCellOrder) {
        foreach ($id in @($rawCellOrder)) {
            if (-not [string]::IsNullOrWhiteSpace([string]$id)) {
                $cellOrder += [string]$id
            }
        }
    }

    $cellDirs = @()
    if (Test-Path $cellsRoot) {
        $cellDirs = Get-ChildItem $cellsRoot -Directory | Sort-Object Name
    }

    Write-Info "cellOrder count: $($cellOrder.Count)"
    Write-Info "cell folder count: $($cellDirs.Count)"

    if ($cellDirs.Count -eq 0) {
        Add-Error $errors "No cell directories were found. Save may not have written cells."
    }

    $folderIds = @($cellDirs | ForEach-Object { $_.Name })
    $seenOrder = @{}
    foreach ($id in $cellOrder) {
        if ($seenOrder.ContainsKey($id)) {
            Add-Error $errors "Duplicate id in project.json cellOrder: $id"
        } else {
            $seenOrder[$id] = $true
        }
        if ($folderIds -notcontains $id) {
            Add-Error $errors "project.json cellOrder references missing cell folder: $id"
        }
    }

    foreach ($folderId in $folderIds) {
        if ($cellOrder.Count -gt 0 -and $cellOrder -notcontains $folderId) {
            Add-Warning $warnings "Cell folder is not present in project.json cellOrder: $folderId"
        }
    }

    $cellSummaries = @()
    $expectedZ = 0
    $idsToInspect = if ($cellOrder.Count -gt 0) { $cellOrder } else { $folderIds }

    foreach ($id in $idsToInspect) {
        $cellFolder = Join-Path $cellsRoot $id
        $cellJsonPath = Join-Path $cellFolder "cell.json"
        if (-not (Test-Path $cellJsonPath)) {
            Add-Error $errors "Missing cell.json for cell: $id"
            continue
        }

        $cell = Read-JsonFile $cellJsonPath
        $cellId = [string](Get-PropertyValue $cell "id" "")
        $name = [string](Get-PropertyValue $cell "name" "")
        $category = [string](Get-PropertyValue $cell "category" "")
        $visible = [bool](Get-PropertyValue $cell "visible" $true)
        $opacity = [double](Get-PropertyValue $cell "opacity" 1.0)
        $zOrder = [int](Get-PropertyValue $cell "zOrder" $expectedZ)

        if ($cellId -ne $id) {
            Add-Warning $warnings "Cell folder id and cell.json id differ: folder=$id json=$cellId"
        }
        if ($opacity -lt 0.0 -or $opacity -gt 1.0) {
            Add-Error $errors "Opacity out of range for $id : $opacity"
        }
        if ($zOrder -ne $expectedZ) {
            Add-Warning $warnings "zOrder is not normalized for $id : zOrder=$zOrder expected=$expectedZ"
        }

        $framesRoot = Join-Path $cellFolder "frames"
        $frameDirs = @()
        if (Test-Path $framesRoot) {
            $frameDirs = Get-ChildItem $framesRoot -Directory | Sort-Object Name
        }
        if ($frameDirs.Count -eq 0) {
            Add-Error $errors "No frames found for cell: $id"
        }

        $frameCount = $frameDirs.Count
        $totalLayerCount = 0
        foreach ($frameDir in $frameDirs) {
            $frameJsonPath = Join-Path $frameDir.FullName "frame.json"
            if (-not (Test-Path $frameJsonPath)) {
                Add-Error $errors "Missing frame.json: $($frameDir.FullName)"
            }
            $layerFiles = Get-ChildItem $frameDir.FullName -File -Filter "layer_*.json" | Sort-Object Name
            if ($layerFiles.Count -eq 0) {
                Add-Error $errors "No layer json files in frame folder: $($frameDir.FullName)"
            }
            $totalLayerCount += $layerFiles.Count
        }

        $cellSummaries += [PSCustomObject]@{
            Order = $expectedZ
            Id = $id
            JsonId = $cellId
            Name = $name
            Category = $category
            Visible = $visible
            Opacity = $opacity
            ZOrder = $zOrder
            Frames = $frameCount
            Layers = $totalLayerCount
        }

        $expectedZ += 1
    }

    Write-Host ""
    Write-Host "Cell summary:" -ForegroundColor Cyan
    $cellSummaries | Format-Table -AutoSize | Out-String | Write-Host

    Write-Host "Audit result:" -ForegroundColor Cyan
    if ($errors.Count -eq 0 -and $warnings.Count -eq 0) {
        Write-Ok "Save/load structure looks consistent."
    } elseif ($errors.Count -eq 0) {
        Write-WarnLine "No fatal errors, but warnings were found: $($warnings.Count)"
    } else {
        Write-ErrLine "Fatal issues were found: $($errors.Count) errors, $($warnings.Count) warnings"
    }

    Write-Host ""
    Write-Host "Recommended manual check after this script:" -ForegroundColor Cyan
    Write-Host "1. Start the app."
    Write-Host "2. Load the same project."
    Write-Host "3. Confirm cell count, names, categories, visible flags, opacity, and order."
    Write-Host "4. Confirm strokes still belong to the correct cell only."
} catch {
    Write-ErrLine $_.Exception.Message
    Write-Host "The terminal was not closed. Fix the issue above and run the script again." -ForegroundColor Yellow
}
