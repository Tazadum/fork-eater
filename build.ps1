# Fork Eater Windows build script (MSVC or MinGW via CMake)
$ErrorActionPreference = "Stop"

$ProjectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ProjectDir

Write-Host "Building Fork Eater..."
Write-Host "Project directory: $ProjectDir"

if (-not (Test-Path "external\imgui")) {
    Write-Host "Dear ImGui not found, cloning..."
    New-Item -ItemType Directory -Force -Path "external" | Out-Null
    git clone https://github.com/ocornut/imgui.git external/imgui
} else {
    Write-Host "Dear ImGui found"
}

New-Item -ItemType Directory -Force -Path "build" | Out-Null
Set-Location build

Write-Host "Configuring with CMake..."
cmake ..

Write-Host "Building..."
cmake --build . --config Release --parallel

Write-Host "Build complete!"
Write-Host ""
$Exe = Join-Path $ProjectDir "build\fork-eater.exe"
if (-not (Test-Path $Exe)) {
    $Exe = Join-Path $ProjectDir "build\Release\fork-eater.exe"
}
Write-Host "To run the shader editor:"
Write-Host "  $Exe"
Write-Host ""
Write-Host "Or from the project root:"
Write-Host "  .\run.ps1"
