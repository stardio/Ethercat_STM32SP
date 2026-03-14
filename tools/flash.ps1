param(
    [string]$Preset = "Debug",
    [string]$BinPath = "",
    [string]$LoaderPath = "gcc\MX66UW1G45G_STM32H7S78-DK.stldr",
    [string]$Address = "0x70000000"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..")
Set-Location $repoRoot

$progPath = "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
if (-not (Test-Path $progPath)) {
    throw "STM32_Programmer_CLI.exe not found: $progPath"
}

if ([string]::IsNullOrWhiteSpace($BinPath)) {
    $BinPath = "build\$Preset\Appli\LCD_Test_Appli.bin"
}

if (-not (Test-Path $BinPath)) {
    throw "Binary not found: $BinPath"
}
if (-not (Test-Path $LoaderPath)) {
    throw "External loader not found: $LoaderPath"
}

Write-Host "[flash] file: $BinPath"
Write-Host "[flash] addr: $Address"
& $progPath -c port=SWD -eL $LoaderPath -d $BinPath $Address -rst
if ($LASTEXITCODE -ne 0) {
    throw "Flash failed."
}

# Record the exact main image that was flashed so UI-only updates can verify compatibility.
$stateDir = Join-Path $repoRoot ".flash_state"
if (-not (Test-Path $stateDir)) {
    New-Item -ItemType Directory -Path $stateDir | Out-Null
}

$statePath = Join-Path $stateDir "appli_last_flash.json"
$binHash = (Get-FileHash $BinPath -Algorithm SHA256).Hash
$state = [ordered]@{
    FlashedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    BinPath = (Resolve-Path $BinPath).Path
    BinHash = $binHash
    Address = $Address
}
$state | ConvertTo-Json | Set-Content -Path $statePath -Encoding UTF8
Write-Host "[flash] state: $statePath"

Write-Host "[flash] done"
