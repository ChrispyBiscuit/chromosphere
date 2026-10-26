# Installs Chromosphere.vst3 for all users. Run from the folder that contains Chromosphere.vst3, in an
# administrator PowerShell:   powershell -ExecutionPolicy Bypass -File install_windows.ps1
$ErrorActionPreference = "Stop"
$src = Join-Path $PSScriptRoot "Chromosphere.vst3"
if (-not (Test-Path $src)) { throw "Chromosphere.vst3 not found next to this script." }
$dst = Join-Path $env:CommonProgramFiles "VST3"
New-Item -ItemType Directory -Force $dst | Out-Null
$target = Join-Path $dst "Chromosphere.vst3"
if (Test-Path $target) { Remove-Item -Recurse -Force $target }
Copy-Item -Recurse $src $target
Write-Host "Installed to $target"
Write-Host "FL Studio: Options > Manage plugins > Find installed plugins (use 'Verify plugins' once), then add Chromosphere from the plugin list."
