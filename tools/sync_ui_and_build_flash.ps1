param(
    [string]$SourceTouchGFX = "Appli\TouchGFX",
    [string]$Preset = "Debug",
    [string]$Target = "LCD_Test_Appli",
    [int]$Jobs = 4,
    [switch]$SkipSync
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..")
Set-Location $repoRoot

$dst = Join-Path $repoRoot "Appli\TouchGFX"
$src = $SourceTouchGFX
if (-not [System.IO.Path]::IsPathRooted($src)) {
    $src = Join-Path $repoRoot $src
}

if ((Resolve-Path $src).Path -eq (Resolve-Path $dst).Path) {
    $SkipSync = $true
    Write-Host "[sync] source and target are same folder; sync skipped"
}

if (-not $SkipSync) {
    if (-not (Test-Path $src)) {
        throw "Designer TouchGFX source not found: $src"
    }
    if (-not (Test-Path $dst)) {
        throw "Target TouchGFX directory not found: $dst"
    }

    Write-Host "[sync] source: $src"
    Write-Host "[sync] target: $dst"
    robocopy $src $dst /MIR /R:1 /W:1 /NFL /NDL /NJH /NJS /NP
    $copyCode = $LASTEXITCODE
    if ($copyCode -gt 7) {
        throw "UI sync failed (robocopy code: $copyCode)"
    }
    Write-Host "[sync] done (robocopy code: $copyCode)"
}

$touchgfxProject = Join-Path $dst "MyApplication_4.touchgfx"
if (-not (Test-Path $touchgfxProject)) {
    throw "TouchGFX project file not found: $touchgfxProject"
}

$generatedRoots = @(
    (Join-Path $dst "generated\gui_generated"),
    (Join-Path $dst "generated\images"),
    (Join-Path $dst "generated\texts"),
    (Join-Path $dst "generated\fonts")
)

$latestGenerated = $null
foreach ($root in $generatedRoots) {
    if (Test-Path $root) {
        $candidate = Get-ChildItem $root -Recurse -File |
            Sort-Object LastWriteTimeUtc -Descending |
            Select-Object -First 1
        if ($candidate -and (-not $latestGenerated -or $candidate.LastWriteTimeUtc -gt $latestGenerated.LastWriteTimeUtc)) {
            $latestGenerated = $candidate
        }
    }
}

if (-not $latestGenerated) {
    throw "No generated TouchGFX files found under $dst\generated"
}

$touchgfxTimeUtc = (Get-Item $touchgfxProject).LastWriteTimeUtc
$genTimeUtc = $latestGenerated.LastWriteTimeUtc
if ($genTimeUtc -lt $touchgfxTimeUtc) {
    Write-Host "[verify] touchgfx: $touchgfxProject ($touchgfxTimeUtc UTC)"
    Write-Host "[verify] latest generated: $($latestGenerated.FullName) ($genTimeUtc UTC)"
    throw "TouchGFX generated files are older than project file. Run Generate Code in Designer for $touchgfxProject and retry."
}

Write-Host "[verify] generated file timestamp OK"
Write-Host "[verify] latest generated: $($latestGenerated.FullName)"

# Always reconfigure after UI sync so generated file lists are refreshed.
Write-Host "[cmake] configure preset: $Preset"
cmake --preset $Preset
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed."
}

& (Join-Path $scriptDir "build_and_flash.ps1") -Preset $Preset -Target $Target -Jobs $Jobs
if ($LASTEXITCODE -ne 0) {
    throw "Build and flash failed."
}

Write-Host "[sync+build+flash] done"
