param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [int]$Jobs = [Environment]::ProcessorCount,
    [switch]$Clean,
    [switch]$Lto
)

$ErrorActionPreference = "Stop"

$Root = $PSScriptRoot
$BuildDir = Join-Path $Root "build"

if ($Jobs -lt 1) {
    throw "Job count must be a positive integer."
}

if ($Clean -and (Test-Path -LiteralPath $BuildDir)) {
    Remove-Item -LiteralPath $BuildDir -Recurse -Force
}

$configureArgs = @(
    "-S", $Root,
    "-B", $BuildDir,
    "-DCMAKE_BUILD_TYPE=$Configuration"
)

if ($Lto) {
    $configureArgs += "-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON"
} else {
    $configureArgs += "-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF"
}

cmake @configureArgs
cmake --build $BuildDir --config $Configuration --parallel $Jobs

$RunningOnWindows = $env:OS -eq "Windows_NT" -or [System.IO.Path]::DirectorySeparatorChar -eq "\"
$ExecutableName = if ($RunningOnWindows) { "jimokomi.exe" } else { "jimokomi" }
$ExecutablePath = Join-Path $BuildDir $ExecutableName
$ConfiguredExecutablePath = Join-Path (Join-Path $BuildDir $Configuration) $ExecutableName
if (-not (Test-Path -LiteralPath $ExecutablePath) -and (Test-Path -LiteralPath $ConfiguredExecutablePath)) {
    $ExecutablePath = $ConfiguredExecutablePath
}
Write-Host "Built $ExecutablePath"
