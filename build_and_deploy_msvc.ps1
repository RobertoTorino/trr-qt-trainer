$ErrorActionPreference = 'Stop'

$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
$qtPrefix = 'C:/Qt/6.11.1/msvc2022_64'
$vsWhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
$sourceDir = $PSScriptRoot
$buildDir = Join-Path $sourceDir 'build-msvc'

if (-not (Test-Path $cmake)) {
    throw "CMake not found: $cmake"
}

if (-not (Test-Path $vsWhere)) {
    throw "vswhere not found: $vsWhere"
}

$vsInstance = & $vsWhere -latest -products Microsoft.VisualStudio.Product.BuildTools -requires Microsoft.Component.MSBuild -property installationPath
if ([string]::IsNullOrWhiteSpace($vsInstance)) {
    throw 'Could not locate a Visual Studio Build Tools installation.'
}

$vsDevCmd = Join-Path $vsInstance 'Common7\Tools\VsDevCmd.bat'
if (-not (Test-Path $vsDevCmd)) {
    throw "VsDevCmd.bat not found: $vsDevCmd"
}

$cachePath = Join-Path $buildDir 'CMakeCache.txt'
if (Test-Path $cachePath) {
    $cacheLine = Select-String -Path $cachePath -Pattern '^CMAKE_HOME_DIRECTORY:INTERNAL=' -SimpleMatch:$false | Select-Object -First 1
    if ($cacheLine) {
        $cachedSource = ($cacheLine.Line -replace '^CMAKE_HOME_DIRECTORY:INTERNAL=', '').Trim()
        $resolvedSourceDir = [System.IO.Path]::GetFullPath($sourceDir)
        $resolvedCachedSource = [System.IO.Path]::GetFullPath($cachedSource)
        if ($resolvedCachedSource -ne $resolvedSourceDir) {
            Write-Host "Detected stale CMake cache source path. Recreating build-msvc..."
            Remove-Item -Recurse -Force $buildDir
        }
    }
}

$buildCommand = "call `"$vsDevCmd`" -arch=amd64 -host_arch=amd64 && `"$cmake`" -S `"$sourceDir`" -B `"$buildDir`" -G `"NMake Makefiles`" -DCMAKE_PREFIX_PATH=`"$qtPrefix`" -DCMAKE_BUILD_TYPE=Release && `"$cmake`" --build `"$buildDir`""

Push-Location $sourceDir
try {
    cmd.exe /c $buildCommand
    & powershell -ExecutionPolicy Bypass -File (Join-Path $sourceDir 'deploy_msvc.ps1')
}
finally {
    Pop-Location
}