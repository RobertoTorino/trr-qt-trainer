$ErrorActionPreference = 'Stop'

$qtRoot = 'C:\Qt\6.11.1\msvc2022_64'
$exePath = Join-Path $PSScriptRoot 'build-msvc\trr_qt_trainer.exe'

if (-not (Test-Path $exePath)) {
    throw "Executable not found: $exePath. Build the Release configuration first."
}

& (Join-Path $qtRoot 'bin\windeployqt.exe') --release --compiler-runtime $exePath