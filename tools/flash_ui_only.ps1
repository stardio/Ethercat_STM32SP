param(
    [string]$Preset = "Debug",
    [string]$ElfPath = "",
    [string]$UiBinPath = "",
    [string]$LoaderPath = "gcc\MX66UW1G45G_STM32H7S78-DK.stldr",
    [string]$Address = "0x70200000",
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..")
Set-Location $repoRoot

$toolBin = "C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\12.2 mpacbti-rel1\bin"
if (Test-Path $toolBin) {
    $env:PATH = "$toolBin;$env:PATH"
}

$progPath = "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
if (-not (Test-Path $progPath)) {
    throw "STM32_Programmer_CLI.exe not found: $progPath"
}

if ([string]::IsNullOrWhiteSpace($ElfPath)) {
    $ElfPath = "build\$Preset\Appli\LCD_Test_Appli.elf"
}

if ([string]::IsNullOrWhiteSpace($UiBinPath)) {
    $UiBinPath = "build\$Preset\Appli\LCD_Test_UI.bin"
}

if (-not (Test-Path $ElfPath)) {
    throw "ELF not found: $ElfPath"
}
if (-not (Test-Path $LoaderPath)) {
    throw "External loader not found: $LoaderPath"
}

Write-Host "[ui] note: UI-only flashes Font/Text/Image assets only."
Write-Host "[ui] note: Screen layout/logic changes require full flash (tools/build_and_flash.ps1)."

if (-not $Force) {
    $statePath = Join-Path $repoRoot ".flash_state\appli_last_flash.json"
    $mainBinPath = Join-Path $repoRoot "build\$Preset\Appli\LCD_Test_Appli.bin"

    if (-not (Test-Path $statePath)) {
        throw "Full-flash state not found. Run tools/build_and_flash.ps1 once, then retry UI-only."
    }
    if (-not (Test-Path $mainBinPath)) {
        throw "Main binary not found: $mainBinPath"
    }

    $state = Get-Content $statePath | ConvertFrom-Json
    $currentMainHash = (Get-FileHash $mainBinPath -Algorithm SHA256).Hash
    if ($state.BinHash -ne $currentMainHash) {
        throw "Main binary differs from last full flash. Run tools/build_and_flash.ps1 before UI-only."
    }

    $lastFlashUtc = [datetime]::Parse($state.FlashedAtUtc)
    $codeRoots = @(
        (Join-Path $repoRoot "Appli\TouchGFX\generated\gui_generated"),
        (Join-Path $repoRoot "Appli\TouchGFX\gui")
    )

    $latestCodeFile = $null
    foreach ($root in $codeRoots) {
        if (Test-Path $root) {
            $candidate = Get-ChildItem $root -Recurse -File |
                Sort-Object LastWriteTimeUtc -Descending |
                Select-Object -First 1
            if ($candidate -and (-not $latestCodeFile -or $candidate.LastWriteTimeUtc -gt $latestCodeFile.LastWriteTimeUtc)) {
                $latestCodeFile = $candidate
            }
        }
    }

    if ($latestCodeFile -and $latestCodeFile.LastWriteTimeUtc -gt $lastFlashUtc) {
        throw "TouchGFX layout/code changed after last full flash ($($latestCodeFile.FullName)). Run tools/build_and_flash.ps1."
    }
}

Write-Host "[ui] extract from: $ElfPath"
arm-none-eabi-objcopy -O binary `
    --only-section=FontFlashSection `
    --only-section=TextFlashSection `
    --only-section=ExtFlashSection `
    $ElfPath $UiBinPath
if ($LASTEXITCODE -ne 0) {
    throw "UI asset extraction failed."
}

if (-not (Test-Path $UiBinPath)) {
    throw "UI binary not created: $UiBinPath"
}

$uiBin = Get-Item $UiBinPath
Write-Host "[ui] bin: $($uiBin.FullName) ($([Math]::Round($uiBin.Length / 1MB, 2)) MB)"

Write-Host "[ui] flash addr: $Address"
& $progPath -c port=SWD -eL $LoaderPath -d $UiBinPath $Address -rst
if ($LASTEXITCODE -ne 0) {
    throw "UI-only flash failed."
}

Write-Host "[ui] done"
