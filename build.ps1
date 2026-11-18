# Builds Zadabar64 (VST3 + Standalone, 64-bit Release).
# Run from PowerShell:   .\build.ps1
# If Windows blocks the script, run once:  Set-ExecutionPolicy -Scope CurrentUser RemoteSigned

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

# Configure (first run downloads JUCE, takes a few minutes).
# CMake picks the newest Visual Studio it finds; -A x64 = 64-bit.
cmake -B build -A x64
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

# Compile
cmake --build build --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

$out = Join-Path $PSScriptRoot "build\Zadabar64_artefacts\Release"
Write-Host ""
Write-Host "Build OK." -ForegroundColor Green
Write-Host "VST3:       $out\VST3\Zadabar64.vst3"
Write-Host "Standalone: $out\Standalone\Zadabar64.exe"
