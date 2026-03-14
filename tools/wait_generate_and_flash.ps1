param(
    [string]$ProjectFile = "Appli\TouchGFX\MyApplication_4.touchgfx",
    [string]$GeneratedRoot = "Appli\TouchGFX\generated",
    [int]$TimeoutSeconds = 600,
    [int]$PollSeconds = 3
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..")
Set-Location $repoRoot

if (-not [System.IO.Path]::IsPathRooted($ProjectFile)) {
    $ProjectFile = Join-Path $repoRoot $ProjectFile
}
if (-not [System.IO.Path]::IsPathRooted($GeneratedRoot)) {
    $GeneratedRoot = Join-Path $repoRoot $GeneratedRoot
}

if (-not (Test-Path $ProjectFile)) {
    throw "Project file not found: $ProjectFile"
}
if (-not (Test-Path $GeneratedRoot)) {
    throw "Generated root not found: $GeneratedRoot"
}

$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
Write-Host "[wait] project: $ProjectFile"
Write-Host "[wait] generated root: $GeneratedRoot"
Write-Host "[wait] timeout: $TimeoutSeconds sec"

while ((Get-Date) -lt $deadline) {
    $projTime = (Get-Item $ProjectFile).LastWriteTimeUtc
    $latestGen = Get-ChildItem $GeneratedRoot -Recurse -File |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1

    if ($latestGen -and $latestGen.LastWriteTimeUtc -ge $projTime) {
        Write-Host "[wait] generate detected: $($latestGen.FullName)"
        Write-Host "[wait] running sync+build+flash..."
        & (Join-Path $scriptDir "sync_ui_and_build_flash.ps1")
        exit $LASTEXITCODE
    }

    Start-Sleep -Seconds $PollSeconds
}

throw "Generate Code was not detected before timeout."
