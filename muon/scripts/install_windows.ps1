# Installs ChromosphereMuon.vst3 (Chromosphere | MUON) for all users. Run from the folder that contains it, in an
# administrator PowerShell:   powershell -ExecutionPolicy Bypass -File install_windows.ps1
$ErrorActionPreference = "Stop"
$src = Join-Path $PSScriptRoot "ChromosphereMuon.vst3"
if (-not (Test-Path $src)) { throw "ChromosphereMuon.vst3 not found next to this script." }
$dst = Join-Path $env:CommonProgramFiles "VST3"
New-Item -ItemType Directory -Force $dst | Out-Null
$target = Join-Path $dst "ChromosphereMuon.vst3"
if (Test-Path $target) { Remove-Item -Recurse -Force $target }
Copy-Item -Recurse $src $target
Write-Host "Installed to $target"
Write-Host "FL Studio: Options > Manage plugins > Find installed plugins, then add 'Chromosphere | MUON' to a mixer insert."
