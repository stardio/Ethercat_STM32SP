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

Write-Host "[flash] done"
