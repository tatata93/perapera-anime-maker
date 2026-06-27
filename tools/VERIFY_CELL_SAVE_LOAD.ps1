param(
    [string]$RepoRoot = "C:\Users\tak01\github\perapera-anime-maker",
    [string]$ProjectDir = ""
)

$ErrorActionPreference = "Continue"
$issues = New-Object System.Collections.Generic.List[string]
$warnings = New-Object System.Collections.Generic.List[string]

function Add-Issue([string]$message) {
    $script:issues.Add($message) | Out-Null
    Write-Host "ERROR: $message" -ForegroundColor Red
}

function Add-WarningLine([string]$message) {
    $script:warnings.Add($message) | Out-Null
    Write-Host "WARN : $message" -ForegroundColor Yellow
}

function Has-JsonProperty($obj, [string]$name) {
    if ($null -eq $obj) { return $false }
    return ($obj.PSObject.Properties.Name -contains $name)
}

function Get-JsonPropertyValue($obj, [string]$name, $defaultValue = $null) {
    if (Has-JsonProperty $obj $name) {
        return $obj.PSObject.Properties[$name].Value
    }
    return $defaultValue
}

function Read-JsonSafe([string]$path, [string]$label) {
    if (-not (Test-Path $path)) {
        Add-Issue "$label not found: $path"
        return $null
    }

    try {
        return Get-Content $path -Raw -Encoding UTF8 | ConvertFrom-Json
    } catch {
        Add-Issue "$label failed to parse JSON: $path :: $($_.Exception.Message)"
        return $null
    }
}

function To-ArraySafe($value) {
    if ($null -eq $value) { return @() }
    if ($value -is [System.Array]) { return @($value) }
    return @($value)
}

function Get-CellDisplayName($cellJson, [string]$fallbackId) {
    $name = [string](Get-JsonPropertyValue $cellJson "name" "")
    if ([string]::IsNullOrWhiteSpace($name)) { return $fallbackId }
    return $name
}

function Test-NumericRange($value, [double]$min, [double]$max, [string]$label) {
    if ($null -eq $value) {
        Add-WarningLine "$label missing"
        return
    }

    $num = 0.0
    if (-not [double]::TryParse([string]$value, [ref]$num)) {
        Add-Issue "$label is not numeric: $value"
        return
    }

    if ($num -lt $min -or $num -gt $max) {
        Add-Issue "$label out of range [$min, $max]: $num"
    }
}

if ([string]::IsNullOrWhiteSpace($ProjectDir)) {
    $ProjectDir = Join-Path $RepoRoot "my_anime_project"
}

Write-Host "=== CellPanel v1.9 save/load audit ===" -ForegroundColor Cyan
Write-Host "RepoRoot  : $RepoRoot"
Write-Host "ProjectDir: $ProjectDir"
Write-Host ""

if (-not (Test-Path $RepoRoot)) {
    Add-Issue "RepoRoot not found: $RepoRoot"
}
if (-not (Test-Path $ProjectDir)) {
    Add-Issue "ProjectDir not found: $ProjectDir"
}

$projectJsonPath = Join-Path $ProjectDir "project.json"
$projectJson = Read-JsonSafe $projectJsonPath "project.json"

$cellsRoot = Join-Path $ProjectDir "cells"
if (-not (Test-Path $cellsRoot)) {
    Add-Issue "cells directory not found: $cellsRoot"
}

$cellDirs = @()
if (Test-Path $cellsRoot) {
    $cellDirs = @(Get-ChildItem $cellsRoot -Directory | Sort-Object Name)
}

$cellOrder = @()
if ($null -ne $projectJson) {
    if (Has-JsonProperty $projectJson "cellOrder") {
        $cellOrder = @(To-ArraySafe (Get-JsonPropertyValue $projectJson "cellOrder" @())) | ForEach-Object { [string]$_ }
    } else {
        Add-WarningLine "project.json has no cellOrder; folder order will be used for audit"
    }
}

Write-Host "=== Project summary ===" -ForegroundColor Cyan
Write-Host "Cell folders: $($cellDirs.Count)"
Write-Host "cellOrder   : $($cellOrder.Count)"

if ($cellDirs.Count -eq 0) {
    Add-Issue "no cell folders found"
}

$folderIds = @($cellDirs | ForEach-Object { $_.Name })

if ($cellOrder.Count -gt 0) {
    foreach ($id in $cellOrder) {
        if ($folderIds -notcontains $id) {
            Add-Issue "cellOrder references missing cell folder: $id"
        }
    }
    foreach ($id in $folderIds) {
        if ($cellOrder -notcontains $id) {
            Add-WarningLine "cell folder is not listed in cellOrder: $id"
        }
    }

    $duplicates = $cellOrder | Group-Object | Where-Object { $_.Count -gt 1 }
    foreach ($dup in $duplicates) {
        Add-Issue "cellOrder contains duplicate id: $($dup.Name)"
    }
}

Write-Host ""
Write-Host "=== Cell summary ===" -ForegroundColor Cyan

$cellRecords = New-Object System.Collections.Generic.List[object]

foreach ($dir in $cellDirs) {
    $cellIdFromFolder = $dir.Name
    $cellJsonPath = Join-Path $dir.FullName "cell.json"
    $cellJson = Read-JsonSafe $cellJsonPath "cell.json for $cellIdFromFolder"
    if ($null -eq $cellJson) { continue }

    $jsonId = [string](Get-JsonPropertyValue $cellJson "id" $cellIdFromFolder)
    if ($jsonId -ne $cellIdFromFolder) {
        Add-WarningLine "cell folder/id mismatch: folder=$cellIdFromFolder json=$jsonId"
    }

    $name = Get-CellDisplayName $cellJson $cellIdFromFolder
    $category = [string](Get-JsonPropertyValue $cellJson "category" "")
    $visible = Get-JsonPropertyValue $cellJson "visible" $true
    $opacity = Get-JsonPropertyValue $cellJson "opacity" $null
    $zOrder = Get-JsonPropertyValue $cellJson "zOrder" $null

    Test-NumericRange $opacity 0.0 1.0 "$cellIdFromFolder opacity"

    if ($null -ne $zOrder) {
        $z = 0
        if (-not [int]::TryParse([string]$zOrder, [ref]$z)) {
            Add-Issue "$cellIdFromFolder zOrder is not integer: $zOrder"
        }
    } else {
        Add-WarningLine "$cellIdFromFolder zOrder missing"
    }

    $framesRoot = Join-Path $dir.FullName "frames"
    if (-not (Test-Path $framesRoot)) {
        Add-Issue "$cellIdFromFolder has no frames directory"
        $frameCount = 0
        $layerCountTotal = 0
    } else {
        $frameDirs = @(Get-ChildItem $framesRoot -Directory | Sort-Object Name)
        $frameCount = $frameDirs.Count
        $layerCountTotal = 0
        if ($frameCount -eq 0) {
            Add-Issue "$cellIdFromFolder has zero frame folders"
        }

        foreach ($frameDir in $frameDirs) {
            $frameJsonPath = Join-Path $frameDir.FullName "frame.json"
            if (-not (Test-Path $frameJsonPath)) {
                Add-Issue "$cellIdFromFolder/$($frameDir.Name) missing frame.json"
            }

            $layerFiles = @(Get-ChildItem $frameDir.FullName -File -Filter "layer_*.json" | Sort-Object Name)
            $layerCountTotal += $layerFiles.Count
            if ($layerFiles.Count -eq 0) {
                Add-WarningLine "$cellIdFromFolder/$($frameDir.Name) has no layer_*.json"
            }

            foreach ($layerFile in $layerFiles) {
                $layerJson = Read-JsonSafe $layerFile.FullName "layer json $($layerFile.Name)"
                if ($null -eq $layerJson) { continue }
                $layerId = [string](Get-JsonPropertyValue $layerJson "id" (Get-JsonPropertyValue $layerJson "layerId" ""))
                if (-not [string]::IsNullOrWhiteSpace($layerId)) {
                    if ($layerId -notlike "*$cellIdFromFolder*") {
                        Add-WarningLine "$cellIdFromFolder/$($frameDir.Name)/$($layerFile.Name) layer id may not be cell-scoped: $layerId"
                    }
                }
            }
        }
    }

    $record = [PSCustomObject]@{
        Id = $cellIdFromFolder
        Name = $name
        Category = $category
        Visible = $visible
        Opacity = $opacity
        Z = $zOrder
        Frames = $frameCount
        LayersTotal = $layerCountTotal
    }
    $cellRecords.Add($record) | Out-Null
}

$cellRecords | Format-Table -AutoSize

if ($cellRecords.Count -gt 0) {
    $zRecords = @($cellRecords | Where-Object { $null -ne $_.Z -and "$($_.Z)" -ne "" } | Sort-Object {[int]$_.Z})
    if ($zRecords.Count -eq $cellRecords.Count) {
        $expected = 0
        foreach ($record in $zRecords) {
            $actual = [int]$record.Z
            if ($actual -ne $expected) {
                Add-WarningLine "zOrder is not contiguous from zero: expected $expected but $($record.Id) has $actual"
            }
            $expected += 1
        }
    }
}

Write-Host ""
Write-Host "=== Result ===" -ForegroundColor Cyan
Write-Host "Errors  : $($issues.Count)"
Write-Host "Warnings: $($warnings.Count)"

if ($issues.Count -eq 0) {
    Write-Host "Audit completed without fatal errors." -ForegroundColor Green
} else {
    Write-Host "Audit found errors. Fix these before moving to timesheet/cell placement work." -ForegroundColor Red
}

if ($warnings.Count -gt 0) {
    Write-Host "Warnings may be acceptable for legacy projects, but review them before committing save/load changes." -ForegroundColor Yellow
}
