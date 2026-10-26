<#
    GRID VOX - one-shot Windows build: VST3 bundle + NSIS installer.

    Prerequisites (all free):
      * Visual Studio 2022 with "Desktop development with C++"
      * CMake 3.22+            (bundled with VS, or https://cmake.org/download)
      * NSIS 3.x               (https://nsis.sourceforge.io/Download)
      * Git                    (only for the JUCE fallback download)

    Usage (from the installer folder, in a normal PowerShell window):
        powershell -ExecutionPolicy Bypass -File .\build-installer.ps1

    Result:
        installer\GRID-VOX-1.0.0-Setup.exe      <- double-click to install
#>

[CmdletBinding()]
param(
    [string] $Version = "1.0.0",
    [string] $JucePath = ""
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Push-Location $PSScriptRoot
try {
    function Find-Tool([string[]] $candidates, [string] $name) {
        foreach ($c in $candidates) { if (Test-Path $c) { return $c } }
        $cmd = Get-Command $name -ErrorAction SilentlyContinue
        if ($cmd) { return $cmd.Source }
        return $null
    }

    # ---- 1. preflight ----------------------------------------------------
    $cmake = Find-Tool @("$env:ProgramFiles\CMake\bin\cmake.exe") "cmake.exe"
    if (-not $cmake) { throw "CMake not found. Install it, or run this from a VS 2022 'x64 Native Tools' prompt." }

    $makensis = Find-Tool @("${env:ProgramFiles(x86)}\NSIS\makensis.exe",
                            "$env:ProgramFiles\NSIS\makensis.exe") "makensis.exe"
    if (-not $makensis) { throw "NSIS not found. Install it from https://nsis.sourceforge.io/Download and re-run." }

    # ---- 2. JUCE ---------------------------------------------------------
    $juceDir = if ($JucePath) { $JucePath } else { Join-Path (Split-Path -Parent $root) "JUCE" }
    if (-not (Test-Path (Join-Path $juceDir "CMakeLists.txt"))) {
        Write-Host "JUCE 8.0.6 not found next to the project - fetching (one time)..." -ForegroundColor Cyan
        git clone --depth 1 --branch 8.0.6 https://github.com/juce-framework/JUCE.git $juceDir
        if ($LASTEXITCODE -ne 0) { throw "Could not clone JUCE. Place a JUCE 8.0.6 checkout next to the GridVox folder." }
    }

    # ---- 3. build the plug-in -------------------------------------------
    $buildDir = Join-Path $root "build-win"
    Write-Host "`n[1/3] Configuring..." -ForegroundColor Cyan
    & $cmake -B $buildDir -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -S $root
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }

    Write-Host "`n[2/3] Building GRID VOX (Release)..." -ForegroundColor Cyan
    & $cmake --build $buildDir --config Release --target GridVox_VST3 --parallel
    if ($LASTEXITCODE -ne 0) { throw "Build failed." }

    $payload = Join-Path $buildDir "GridVox_artefacts\Release\VST3"
    if (-not (Test-Path (Join-Path $payload "GRID VOX.vst3"))) { throw "Build finished but no bundle in $payload" }

    # ---- 4. wrap it in the installer -------------------------------------
    Write-Host "`n[3/3] Building installer with NSIS..." -ForegroundColor Cyan
    & $makensis /V2 "/DVERSION=$Version" "/DSRCDIR=$payload" (Join-Path $PSScriptRoot "GridVox-setup.nsi")
    if ($LASTEXITCODE -ne 0) { throw "makensis failed." }

    $exe = Join-Path $PSScriptRoot "GRID-VOX-$Version-Setup.exe"
    if (-not (Test-Path $exe)) { throw "Installer was not produced." }

    Write-Host "`nDone." -ForegroundColor Green
    Write-Host "  Plug-in   : $payload\GRID VOX.vst3"
    Write-Host "  Installer : $exe"
    Write-Host "`nRun the installer, then rescan plug-ins in your DAW." -ForegroundColor Green
    Start-Process explorer.exe "/select,`"$exe`""
}
finally {
    Pop-Location
}
