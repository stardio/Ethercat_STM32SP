param(
    [string]$Preset = "Debug",
    [string]$Target = "LCD_Test_Appli",
    [int]$Jobs = 4,
    [string]$Address = "0x70000000"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

& (Join-Path $scriptDir "build.ps1") -Preset $Preset -Target $Target -Jobs $Jobs
if ($LASTEXITCODE -ne 0) {
    throw "Build step failed."
}

$binPath = "build\$Preset\Appli\$Target.bin"
& (Join-Path $scriptDir "flash.ps1") -Preset $Preset -BinPath $binPath -Address $Address
if ($LASTEXITCODE -ne 0) {
    throw "Flash step failed."
}

Write-Host "[build+flash] done"
