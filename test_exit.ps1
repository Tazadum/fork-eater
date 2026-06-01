# Fork Eater exit functionality tests (Windows)
$ErrorActionPreference = "Stop"

$ProjectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ProjectDir

$Binary = Join-Path $ProjectDir "build\fork-eater.exe"
if (-not (Test-Path $Binary)) {
    $Binary = Join-Path $ProjectDir "build\Release\fork-eater.exe"
}
if (-not (Test-Path $Binary)) {
    Write-Host "fork-eater.exe not found. Build first with .\build.ps1"
    exit 1
}

Write-Host "Testing Fork Eater Exit Functionality..."
Write-Host "============================================="

function Test-ExitCode {
    param($Args, $Expected)
    & $Binary @Args 2>$null | Out-Null
    $code = $LASTEXITCODE
    if ($null -eq $code) { $code = 0 }
    return $code -eq $Expected
}

function Invoke-ForkEater {
    param([string[]]$ForkArgs)
    $outLog = Join-Path $env:TEMP "fork-eater-test-out.log"
    $errLog = Join-Path $env:TEMP "fork-eater-test-err.log"
    $proc = Start-Process -FilePath $Binary -ArgumentList $ForkArgs -Wait -PassThru `
        -WindowStyle Hidden -RedirectStandardOutput $outLog -RedirectStandardError $errLog
    return $proc.ExitCode
}

Write-Host "1. Testing --help command..."
if ((Invoke-ForkEater @("--help")) -ne 0) { Write-Host "   Help command failed"; exit 1 }
Write-Host "   Help command works"

Write-Host "2. Testing --test mode (default exit code)..."
if ((Invoke-ForkEater @("--test")) -ne 0) { Write-Host "   Test mode failed"; exit 1 }
Write-Host "   Test mode works with exit code 0"

Write-Host "3. Testing --test mode (custom exit code 42)..."
$exit42 = Invoke-ForkEater @("--test", "42")
if ($exit42 -ne 42) {
    Write-Host "   Test mode failed with custom exit code (got $exit42)"
    exit 1
}
Write-Host "   Test mode works with custom exit code 42"

Write-Host "4. Testing application startup and termination..."
$proc = Start-Process -FilePath $Binary -ArgumentList "project\test" -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 2
if (-not $proc.HasExited) {
    Write-Host "   Application starts successfully"
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    Write-Host "   Application terminates properly"
} else {
    Write-Host "   Application failed to start or exited early"
    exit 1
}

Write-Host "============================================="
Write-Host "All exit functionality tests passed!"
