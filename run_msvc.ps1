$ErrorActionPreference = 'Stop'

$exePath = Join-Path $PSScriptRoot 'build-msvc\trr_qt_trainer.exe'

if (-not (Test-Path $exePath)) {
    throw "Executable not found: $exePath. Run build_and_deploy_msvc.ps1 first."
}

& $exePath