param([switch]$CheckOnly)
$ErrorActionPreference = "Stop"
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Version = Get-Content (Join-Path $Here "version.json") -Raw | ConvertFrom-Json
$Repo = "aidenjantzi1-ctrl/GREEN-ION"
$Api = "https://api.github.com/repos/$Repo/releases?per_page=20"
$StateRoot = Join-Path $env:LOCALAPPDATA "GREEN_ION"
$AppRoot = Join-Path $StateRoot "App"
$CurrentFile = Join-Path $StateRoot "current.txt"
$PreviousFile = Join-Path $StateRoot "previous.txt"
New-Item -ItemType Directory -Force -Path $AppRoot | Out-Null

Write-Host "[UPDATE] Checking official GREEN ION V16 GitHub releases..."
$Headers = @{ "Accept"="application/vnd.github+json"; "User-Agent"="GREEN-ION-V16-Updater" }
$Releases = Invoke-RestMethod -Uri $Api -Headers $Headers -Method Get
$Release = $Releases | Where-Object {
  -not $_.draft -and $_.tag_name -match '^v16\.' -and
  ($_.assets.name -contains "GREEN_ION_V16_WINDOWS.zip") -and
  ($_.assets.name -contains "GREEN_ION_V16_WINDOWS.sha256")
} | Select-Object -First 1
if (-not $Release) {
  Write-Host "[UPDATE] No official V16 Windows release package is published yet."
  exit 0
}

$Tag = [string]$Release.tag_name
Write-Host "[UPDATE] Latest official Windows package: $Tag"
if ($Release.name) { Write-Host "[UPDATE] Release: $($Release.name)" }
if ($Release.published_at) { Write-Host "[UPDATE] Published: $($Release.published_at)" }
if ($Release.body) {
  $Notes = ([string]$Release.body -replace '[\r\n]+',' ' -replace '\s+',' ').Trim()
  if ($Notes.Length -gt 420) { $Notes = $Notes.Substring(0,420) + "..." }
  Write-Host "[UPDATE] Notes: $Notes"
}
Write-Host "[UPDATE] This installed package reports: $($Version.version)"
$InstalledTag = "v" + [string]$Version.version
if ($Tag -eq $InstalledTag) {
  Write-Host "[UPDATE] This is already the newest published V16 Windows version."
  exit 0
}
if ($CheckOnly) { exit 0 }

$ZipAsset = $Release.assets | Where-Object name -eq "GREEN_ION_V16_WINDOWS.zip" | Select-Object -First 1
$ShaAsset = $Release.assets | Where-Object name -eq "GREEN_ION_V16_WINDOWS.sha256" | Select-Object -First 1
$TempRoot = Join-Path $env:TEMP ("green-ion-update-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $TempRoot | Out-Null
try {
  $Zip = Join-Path $TempRoot "GREEN_ION_V16_WINDOWS.zip"
  $Sha = Join-Path $TempRoot "GREEN_ION_V16_WINDOWS.sha256"
  Invoke-WebRequest -Uri $ZipAsset.browser_download_url -Headers $Headers -OutFile $Zip
  Invoke-WebRequest -Uri $ShaAsset.browser_download_url -Headers $Headers -OutFile $Sha
  $Expected = ((Get-Content $Sha -Raw) -split '\s+')[0].Trim().ToLowerInvariant()
  if ($Expected -notmatch '^[0-9a-f]{64}$') { throw "Release checksum file is invalid." }
  $Actual = (Get-FileHash -Algorithm SHA256 $Zip).Hash.ToLowerInvariant()
  if ($Actual -ne $Expected) { throw "Downloaded Windows package SHA-256 does not match the official checksum." }

  $Stage = Join-Path $TempRoot "expanded"
  Expand-Archive -Path $Zip -DestinationPath $Stage -Force
  $SourceRoot = Join-Path $Stage "GREEN_ION_V16_WINDOWS"
  foreach ($Required in @("START_GREEN_ION.cmd","Ground_Station\ground_station.py","Ground_Station\version.json","web\index.html")) {
    if (-not (Test-Path (Join-Path $SourceRoot $Required))) { throw "Verified ZIP is missing $Required" }
  }

  $Target = Join-Path $AppRoot ($Tag + "-" + $Actual.Substring(0,12))
  if (-not (Test-Path $Target)) {
    Move-Item -Path $SourceRoot -Destination $Target
  } else {
    Write-Host "[UPDATE] This exact verified package is already installed; leaving its files untouched."
  }
  if (Test-Path $CurrentFile) {
    $OldCurrent = (Get-Content $CurrentFile -Raw).Trim()
    if ($OldCurrent -and $OldCurrent -ne $Target -and (Test-Path $OldCurrent)) {
      Set-Content -Path $PreviousFile -Value $OldCurrent -Encoding UTF8
    }
  }
  Set-Content -Path $CurrentFile -Value $Target -Encoding UTF8
  Write-Host "[UPDATE] Selected verified $Tag side-by-side at:"
  Write-Host "         $Target"
  Write-Host "[UPDATE] Your GREEN_ION_Data folder was not changed."
  if (Test-Path $PreviousFile) { Write-Host "[UPDATE] Previous app version is retained for one-click rollback." }
  Write-Host "[UPDATE] Close the currently running ground-station window, then run START_GREEN_ION.cmd again."
} finally {
  Remove-Item -Recurse -Force $TempRoot -ErrorAction SilentlyContinue
}
