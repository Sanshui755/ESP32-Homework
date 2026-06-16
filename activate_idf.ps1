param(
    [string]$IdfVersion = "v6.0.1"
)

$ErrorActionPreference = "Continue"

$EspIdfRoot = "C:\Esp\$IdfVersion\esp-idf"
$IdfToolsRoot = "C:\Users\27928\.espressif\tools"
$PythonEnvRoot = "C:\Users\27928\.espressif\python_env"

if (-not (Test-Path $EspIdfRoot)) {
    Write-Host -ForegroundColor Red "ERROR: ESP-IDF not found at $EspIdfRoot"
    Write-Host -ForegroundColor Yellow "Available versions:"
    Get-ChildItem "C:\Esp" -Directory -ErrorAction SilentlyContinue | ForEach-Object { Write-Host "   $($_.Name)" }
    return
}

$env:IDF_PATH = $EspIdfRoot
$env:IDF_TOOLS_PATH = "C:\Users\27928\.espressif"
Write-Host -ForegroundColor Green "IDF_PATH = $env:IDF_PATH"

$toolDirs = @(
    (Get-ChildItem "$IdfToolsRoot\cmake" -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1).FullName + "\bin",
    (Get-ChildItem "$IdfToolsRoot\ninja" -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1).FullName,
    (Get-ChildItem "$IdfToolsRoot\ccache" -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1).FullName + "\bin",
    (Get-ChildItem "$IdfToolsRoot\riscv32-esp-elf" -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1).FullName + "\riscv32-esp-elf\bin",
    (Get-ChildItem "$IdfToolsRoot\xtensa-esp-elf" -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1).FullName + "\xtensa-esp-elf\bin",
    (Get-ChildItem "$IdfToolsRoot\openocd-esp32" -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1).FullName + "\bin",
    (Get-ChildItem "$IdfToolsRoot\dfu-util" -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1).FullName
)

$venvDir = Get-ChildItem $PythonEnvRoot -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1
if ($venvDir) {
    $venvScripts = Join-Path $venvDir.FullName "Scripts"
    if (Test-Path $venvScripts) {
        $toolDirs = ,$venvScripts + $toolDirs
        Write-Host -ForegroundColor Green "Python venv: $($venvDir.Name)"
    }
}

$addedPaths = @()
foreach ($dir in $toolDirs) {
    if ($dir -and (Test-Path $dir) -and $env:PATH -notlike "*$dir*") {
        $env:PATH = "$dir;$env:PATH"
        $addedPaths += Split-Path $dir -Leaf
    }
}
if ($addedPaths.Count -gt 0) {
    Write-Host -ForegroundColor Green "Added to PATH: $($addedPaths -join ', ')"
}

function idf.py {
    py -3 "$env:IDF_PATH\tools\idf.py" @args
}

Write-Host ""
Write-Host -ForegroundColor Cyan "================================================="
Write-Host -ForegroundColor Cyan "  ESP-IDF $IdfVersion activated in Trae Terminal"
Write-Host -ForegroundColor Cyan "================================================="
Write-Host ""
Write-Host -ForegroundColor Yellow "Available commands:"
Write-Host "  idf.py build         - Build the project"
Write-Host "  idf.py flash         - Flash to device"
Write-Host "  idf.py monitor       - Serial monitor"
Write-Host "  idf.py fullclean     - Clean all build artifacts"
Write-Host "  idf.py menuconfig    - Configure project"
Write-Host ""
Write-Host -ForegroundColor Yellow "To switch version: .\activate_idf.ps1 v5.5.4"
Write-Host ""
