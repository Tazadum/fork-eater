# Fork Eater Windows run script
$ErrorActionPreference = "Stop"

$ProjectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ProjectDir

$Binary = Join-Path $ProjectDir "build\fork-eater.exe"
if (-not (Test-Path $Binary)) {
    $Binary = Join-Path $ProjectDir "build\Release\fork-eater.exe"
}
if (-not (Test-Path $Binary)) {
    Write-Host "fork-eater.exe not found. Build first with:"
    Write-Host "  .\build.ps1"
    exit 1
}

Write-Host "Launching Fork Eater..."
Write-Host "Tip: Press ESC or use File -> Exit to exit gracefully"

& $Binary @args
