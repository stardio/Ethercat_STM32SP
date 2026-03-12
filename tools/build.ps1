param(
    [string]$Preset = "Debug",
    [string]$Target = "LCD_Test_Appli",
    [int]$Jobs = 4
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..")
Set-Location $repoRoot

$toolBin = "C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\12.2 mpacbti-rel1\bin"
if (Test-Path $toolBin) {
    $env:PATH = "$toolBin;$env:PATH"
}

$buildDir = Join-Path $repoRoot "build\$Preset"
if (-not (Test-Path $buildDir)) {
    Write-Host "[build] configure preset: $Preset"
    cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed."
    }
}

Write-Host "[build] target: $Target (jobs=$Jobs)"
if ($Jobs -gt 0) {
    cmake --build $buildDir --target $Target --parallel $Jobs
} else {
    cmake --build $buildDir --target $Target
}
if ($LASTEXITCODE -ne 0) {
    throw "Build failed."
}

$binPath = Join-Path $buildDir "Appli\$Target.bin"
if (Test-Path $binPath) {
    $bin = Get-Item $binPath
    Write-Host "[build] bin: $($bin.FullName) ($([Math]::Round($bin.Length / 1MB, 2)) MB)"
} else {
    Write-Host "[build] warning: bin not found at $binPath"
}

Write-Host "[build] done"
