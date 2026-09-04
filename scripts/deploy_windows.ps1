[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BuildDirectory,
    [string]$Configuration = "Release",
    [string]$Destination = "dist",
    [Parameter(Mandatory = $true)]
    [string]$QtBin,
    [Parameter(Mandatory = $true)]
    [string]$OpenCvBin,
    [Parameter(Mandatory = $true)]
    [string]$OnnxRuntimeRoot,
    [string]$ModelRoot = "models"
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$resolvedBuild = (Resolve-Path -LiteralPath $BuildDirectory).Path
$resolvedQtBin = (Resolve-Path -LiteralPath $QtBin).Path
$resolvedOpenCvBin = (Resolve-Path -LiteralPath $OpenCvBin).Path
$resolvedOnnxRoot = (Resolve-Path -LiteralPath $OnnxRuntimeRoot).Path
$resolvedModels = (Resolve-Path -LiteralPath $ModelRoot).Path

$candidateExecutables = @(
    (Join-Path $resolvedBuild "desktop\Qt\$Configuration\odf_desktop.exe"),
    (Join-Path $resolvedBuild "desktop\Qt\odf_desktop.exe")
)
$executable = $candidateExecutables | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $executable) { throw "odf_desktop.exe was not found under $resolvedBuild" }

$destinationPath = [System.IO.Path]::GetFullPath((Join-Path $projectRoot $Destination))
if ([System.IO.Path]::GetPathRoot($destinationPath) -eq $destinationPath) {
    throw "Refusing to deploy to a filesystem root: $destinationPath"
}
New-Item -ItemType Directory -Force -Path $destinationPath | Out-Null
Copy-Item -LiteralPath $executable -Destination $destinationPath -Force

$onnxDll = Join-Path $resolvedOnnxRoot "lib\onnxruntime.dll"
if (-not (Test-Path -LiteralPath $onnxDll)) { $onnxDll = Join-Path $resolvedOnnxRoot "bin\onnxruntime.dll" }
if (-not (Test-Path -LiteralPath $onnxDll)) { throw "onnxruntime.dll was not found" }
Copy-Item -LiteralPath $onnxDll -Destination $destinationPath -Force

$opencvDll = Get-ChildItem -LiteralPath $resolvedOpenCvBin -Filter "opencv_world*.dll" |
    Where-Object { $_.Name -notmatch "d\.dll$" } | Select-Object -First 1
if (-not $opencvDll) { throw "A Release opencv_world DLL was not found in $resolvedOpenCvBin" }
Copy-Item -LiteralPath $opencvDll.FullName -Destination $destinationPath -Force

$deployedExecutable = Join-Path $destinationPath "odf_desktop.exe"
$windeployqt = Join-Path $resolvedQtBin "windeployqt.exe"
if (-not (Test-Path -LiteralPath $windeployqt)) { throw "windeployqt.exe was not found in $resolvedQtBin" }
& $windeployqt --release --no-translations $deployedExecutable
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed with exit code $LASTEXITCODE" }

Copy-Item -LiteralPath $resolvedModels -Destination (Join-Path $destinationPath "models") -Recurse -Force
Write-Host "Deployment ready: $destinationPath"

