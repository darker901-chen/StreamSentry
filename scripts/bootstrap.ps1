# bootstrap.ps1 — 環境自動安裝(由 Claude Code 執行)
# 用法(系統管理員 PowerShell):  powershell -ExecutionPolicy Bypass -File scripts\bootstrap.ps1
# 目的:盡可能自動裝齊 build 依賴;無法自動的部分明確印出給人處理。

$ErrorActionPreference = 'Stop'
Write-Host "=== OBS plugin bootstrap ===" -ForegroundColor Cyan

function Have($cmd) { return [bool](Get-Command $cmd -ErrorAction SilentlyContinue) }

# --- winget 檢查 ---
if (-not (Have winget)) {
    Write-Host "[MANUAL] 找不到 winget。請從 Microsoft Store 安裝 'App Installer' 後重跑本腳本。" -ForegroundColor Yellow
    exit 1
}

# --- Git ---
if (Have git) { Write-Host "[OK] Git 已安裝" -ForegroundColor Green }
else {
    Write-Host "[..] 安裝 Git" -ForegroundColor Cyan
    winget install --id Git.Git -e --source winget --accept-package-agreements --accept-source-agreements
}

# --- CMake ---
if (Have cmake) { Write-Host "[OK] CMake 已安裝" -ForegroundColor Green }
else {
    Write-Host "[..] 安裝 CMake" -ForegroundColor Cyan
    winget install --id Kitware.CMake -e --source winget --accept-package-agreements --accept-source-agreements
}

# --- Visual Studio Build Tools (C++) ---
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$haveCpp = $false
if (Test-Path $vswhere) {
    $inst = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
    if ($inst) { $haveCpp = $true }
}
if ($haveCpp) { Write-Host "[OK] VS C++ toolchain 已安裝" -ForegroundColor Green }
else {
    Write-Host "[..] 安裝 VS 2022 Build Tools + C++ workload(靜默,可能數分鐘)" -ForegroundColor Cyan
    Write-Host "     若彈出授權同意視窗,請點同意——這一步是 Microsoft 強制的 GUI,無法完全靜默。" -ForegroundColor Yellow
    winget install --id Microsoft.VisualStudio.2022.BuildTools -e --source winget `
        --accept-package-agreements --accept-source-agreements `
        --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
}

# --- OBS Studio ---
$obs = "${env:ProgramFiles}\obs-studio\bin\64bit\obs64.exe"
if (Test-Path $obs) { Write-Host "[OK] OBS Studio 已安裝" -ForegroundColor Green }
else {
    Write-Host "[..] 安裝 OBS Studio" -ForegroundColor Cyan
    winget install --id OBSProject.OBSStudio -e --source winget --accept-package-agreements --accept-source-agreements
}

Write-Host ""
Write-Host "=== bootstrap 完成 ===" -ForegroundColor Cyan
Write-Host "[MANUAL 提醒] 接下來仍需人做的兩件事:" -ForegroundColor Yellow
Write-Host "  1. 若剛裝 VS Build Tools,關掉這個視窗、開一個「新的」PowerShell,環境變數才會生效。" -ForegroundColor Yellow
Write-Host "  2. OBS 內確認 filter 是否出現,必須人肉開 OBS 看(M0 的驗收)。" -ForegroundColor Yellow
Write-Host ""
Write-Host "版本檢查:" -ForegroundColor Cyan
if (Have git)   { git --version }
if (Have cmake) { cmake --version | Select-Object -First 1 }
