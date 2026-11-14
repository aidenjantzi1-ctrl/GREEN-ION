param(
  [switch]$NoRedirect,
  [Parameter(ValueFromRemainingArguments=$true)]
  [string[]]$GroundStationArgs
)
$ErrorActionPreference = "Stop"
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$PackageRoot = Split-Path -Parent $Here
$StateRoot = Join-Path $env:LOCALAPPDATA "GREEN_ION"
$CurrentFile = Join-Path $StateRoot "current.txt"

function Same-Path([string]$A,[string]$B) {
  try { return [System.IO.Path]::GetFullPath($A).TrimEnd('\') -ieq [System.IO.Path]::GetFullPath($B).TrimEnd('\') }
  catch { return $false }
}

if (-not $NoRedirect -and (Test-Path $CurrentFile)) {
  $CurrentRoot = (Get-Content $CurrentFile -Raw).Trim()
  $CurrentScript = Join-Path $CurrentRoot "Ground_Station\Start-GreenIon.ps1"
  if ((Test-Path $CurrentScript) -and -not (Same-Path $CurrentScript $MyInvocation.MyCommand.Path)) {
    & $CurrentScript -NoRedirect @GroundStationArgs
    exit $LASTEXITCODE
  }
}

$VersionFile = Join-Path $Here "version.json"
if (-not (Test-Path $VersionFile)) { throw "GREEN ION version.json is missing." }
$Version = Get-Content $VersionFile -Raw | ConvertFrom-Json

if (-not $env:GREEN_ION_DATA_DIR) {
  $env:GREEN_ION_DATA_DIR = Join-Path $HOME "GREEN_ION_Data"
}
New-Item -ItemType Directory -Force -Path $env:GREEN_ION_DATA_DIR | Out-Null

$PythonExe = $null
$PythonPrefix = @()
if (Get-Command py -ErrorAction SilentlyContinue) {
  $PythonExe = "py"; $PythonPrefix = @("-3")
} elseif (Get-Command python -ErrorAction SilentlyContinue) {
  $PythonExe = "python"
} else {
  Write-Host ""
  Write-Host "[ERROR] Python 3 is required for the Windows ground station."
  Write-Host "Install Python 3 from python.org or the Microsoft Store, then run START_GREEN_ION.cmd again."
  Read-Host "Press Enter to close"
  exit 1
}

& $PythonExe @PythonPrefix -c "import sys; raise SystemExit(0 if sys.version_info >= (3,10) else 1)"
if ($LASTEXITCODE -ne 0) {
  Write-Host "[ERROR] GREEN ION requires Python 3.10 or newer."
  Read-Host "Press Enter to close"
  exit 1
}

$WifiOnly = $false
& $PythonExe @PythonPrefix -c "import serial, serial.tools.list_ports" 2>$null
if ($LASTEXITCODE -ne 0) {
  Write-Host "[SETUP] Installing the pinned USB serial dependency (pyserial 3.5)..."
  & $PythonExe @PythonPrefix -m pip install --user --disable-pip-version-check -r (Join-Path $Here "requirements.txt")
  & $PythonExe @PythonPrefix -c "import serial, serial.tools.list_ports" 2>$null
  if ($LASTEXITCODE -ne 0) {
    $WifiOnly = $true
    Write-Host "[WARN] pyserial is unavailable. GREEN ION will still start in Feather Wi-Fi-only mode."
    Write-Host "       Uno/LoRa USB support will become available after pyserial can be installed."
  }
}

Write-Host ""
Write-Host "GREEN ION V17 Windows Ground Station $($Version.version)"
Write-Host "Data directory: $env:GREEN_ION_DATA_DIR"
Write-Host ""
Write-Host "Simple setup:"
Write-Host "  1. Leave this window open; the blue dashboard opens automatically."
Write-Host "  2. Plug the Uno in any time. GREEN ION keeps watching USB and reconnects automatically."
Write-Host "  3. Join the Feather GREEN-ION-V17 Wi-Fi (or compatible older firmware Wi-Fi) when live status, tests or downloads are needed."
Write-Host "  4. Your saved flights stay in GREEN_ION_Data even when the app is updated or rolled back."
Write-Host "No COM prompt is required."
Write-Host ""

$LaunchArgs = @((Join-Path $Here "ground_station.py"))
if ($WifiOnly) { $LaunchArgs += "--wifi-only" }
if ($GroundStationArgs) { $LaunchArgs += $GroundStationArgs }
& $PythonExe @PythonPrefix @LaunchArgs
exit $LASTEXITCODE
