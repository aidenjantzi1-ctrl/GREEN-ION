$ErrorActionPreference = "Stop"
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$StateRoot = Join-Path $env:LOCALAPPDATA "GREEN_ION"
$CurrentFile = Join-Path $StateRoot "current.txt"
$PreviousFile = Join-Path $StateRoot "previous.txt"

function Valid-GreenIonPackage([string]$Root) {
  if (-not $Root -or -not (Test-Path $Root)) { return $false }
  foreach ($Required in @(
    "START_GREEN_ION.cmd",
    "Ground_Station\ground_station.py",
    "Ground_Station\version.json",
    "web\index.html"
  )) {
    if (-not (Test-Path (Join-Path $Root $Required))) { return $false }
  }
  return $true
}

if (-not (Test-Path $PreviousFile)) {
  Write-Host "[ROLLBACK] No previous GREEN ION app version is recorded."
  exit 0
}
$Previous = (Get-Content $PreviousFile -Raw).Trim()
if (-not (Valid-GreenIonPackage $Previous)) {
  throw "Previous GREEN ION package is missing or incomplete. Current version was not changed."
}
$Current = ""
if (Test-Path $CurrentFile) { $Current = (Get-Content $CurrentFile -Raw).Trim() }
Set-Content -Path $CurrentFile -Value $Previous -Encoding UTF8
if (Valid-GreenIonPackage $Current) {
  Set-Content -Path $PreviousFile -Value $Current -Encoding UTF8
} else {
  Remove-Item $PreviousFile -Force -ErrorAction SilentlyContinue
}
Write-Host "[ROLLBACK] Selected previous verified GREEN ION app:"
Write-Host "           $Previous"
Write-Host "[ROLLBACK] GREEN_ION_Data was not changed."
Write-Host "[ROLLBACK] Close the running app and start GREEN ION again."
