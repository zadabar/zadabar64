# Zadabar64 - builds a release and packs the ZIP people download.
#
# Run from PowerShell in the repo folder:
#     .\release\make-release-zip.ps1            # build Release, then pack
#     .\release\make-release-zip.ps1 -NoBuild   # pack what is already built
#
# Needs: Visual Studio 2022/2026 with C++ and CMake (same as build.ps1). Nothing else.
#
# Result in release\Output\:
#     Zadabar64-<version>-Windows-x64.zip          the download
#     Zadabar64-<version>-Windows-x64.zip.sha256   its checksum (for the release notes)
#
# The ZIP contains one folder with: Zadabar64.vst3 (folder), Zadabar64.clap,
# Zadabar64.exe, INSTALL.txt, README.txt, LICENSE.txt, THIRD_PARTY_NOTICES.txt
# and licenses\. The version comes from project(Zadabar64 VERSION x.y.z) in CMakeLists.txt.

param([switch]$NoBuild)

$ErrorActionPreference = "Stop"

# Links written into the text files. Change them here if the addresses change.
$GitHub  = "https://github.com/zadabar/zadabar64"
$Website = "https://zadabar.com"

$repo = Split-Path $PSScriptRoot -Parent
Set-Location $repo

# --- version
$match = Select-String -Path "CMakeLists.txt" -Pattern 'project\(Zadabar64 VERSION ([0-9]+\.[0-9]+\.[0-9]+)'
if (-not $match) { throw "Could not read the version from CMakeLists.txt" }
$version = $match.Matches[0].Groups[1].Value
Write-Host ""
Write-Host "  ZADABAR64 $version - release ZIP" -ForegroundColor Cyan
Write-Host ""

# --- build (VST3, CLAP, standalone)
if (-not $NoBuild) {
    cmake -B build -A x64 -DZADABAR_CLAP=ON
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }
    cmake --build build --config Release --parallel
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }
}

$built = Join-Path $repo "build\Zadabar64_artefacts\Release"
$vst3  = Join-Path $built "VST3\Zadabar64.vst3"
$clap  = Join-Path $built "CLAP\Zadabar64.clap"
$exe   = Join-Path $built "Standalone\Zadabar64.exe"
foreach ($f in @($vst3, $clap, $exe)) {
    if (-not (Test-Path $f)) { throw "Missing build output: $f  (build first, without -NoBuild)" }
}
if (-not (Test-Path (Join-Path $vst3 "Contents\x86_64-win\Zadabar64.vst3"))) {
    throw "The VST3 bundle is incomplete: $vst3"
}

# --- stage the folder that goes into the ZIP
$name  = "Zadabar64-$version-Windows-x64"
$out   = Join-Path $PSScriptRoot "Output"
$stage = Join-Path $out $name
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage | Out-Null

Copy-Item $vst3 $stage -Recurse
Copy-Item $clap $stage
Copy-Item $exe  $stage

# Text files: fill in version and links, Windows line endings, plain ASCII (Notepad-safe).
function Write-ReleaseText([string]$source, [string]$target) {
    $text = [IO.File]::ReadAllText($source)
    $text = $text.Replace("{VERSION}", $version).Replace("{GITHUB}", $GitHub).Replace("{WEBSITE}", $Website)
    $text = $text -replace "`r?`n", "`r`n"
    [IO.File]::WriteAllText($target, $text, [Text.Encoding]::ASCII)
}
Write-ReleaseText (Join-Path $PSScriptRoot "INSTALL.txt")             (Join-Path $stage "INSTALL.txt")
Write-ReleaseText (Join-Path $PSScriptRoot "README.txt")              (Join-Path $stage "README.txt")
Write-ReleaseText (Join-Path $PSScriptRoot "THIRD_PARTY_NOTICES.txt") (Join-Path $stage "THIRD_PARTY_NOTICES.txt")
Write-ReleaseText (Join-Path $repo "LICENSE")                         (Join-Path $stage "LICENSE.txt")
Copy-Item (Join-Path $PSScriptRoot "licenses") $stage -Recurse

# --- zip (one top folder inside, so "Extract All" gives a tidy folder) + checksum
$zip = Join-Path $out "$name.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
# Not Compress-Archive: Windows PowerShell 5.1 writes "\" into the paths inside the zip,
# so 7-Zip, macOS and Linux unpack loose files instead of folders. Entries get "/".
Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
$root = (Get-Item $stage).Parent.FullName.TrimEnd('\', '/')
$stream = [System.IO.File]::Open($zip, [System.IO.FileMode]::CreateNew)
$archive = New-Object System.IO.Compression.ZipArchive($stream, [System.IO.Compression.ZipArchiveMode]::Create)
try {
    Get-ChildItem $stage -Recurse -File -Force | Sort-Object FullName | ForEach-Object {
        $entry = $_.FullName.Substring($root.Length + 1).Replace('\', '/')
        [void][System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $archive, $_.FullName, $entry, [System.IO.Compression.CompressionLevel]::Optimal)
    }
} finally {
    $archive.Dispose()
    $stream.Dispose()
}
$hash = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLower()
"$hash  $name.zip" | Set-Content "$zip.sha256" -Encoding ASCII
Remove-Item $stage -Recurse -Force

Write-Host ""
Write-Host "  READY." -ForegroundColor Green
Write-Host "  $zip  ($([math]::Round((Get-Item $zip).Length / 1MB, 1)) MB)"
Write-Host "  SHA-256 $hash"
Write-Host ""
