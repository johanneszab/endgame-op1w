<#
.SYNOPSIS
  Capture a firmware update session. Unfiltered, on purpose.

.DESCRIPTION
  capture.ps1 restricts itself to one USB device address, which is right for
  settings changes and WRONG here: a firmware update reboots the mouse (and
  possibly the dongle) into a bootloader, which re-enumerates as a DIFFERENT
  device with a different address and usually a different VID/PID. A device
  filter would silently drop the entire update.

  So this captures every device on the bus, including ones that appear after
  the capture starts, with a large kernel buffer because firmware payloads are
  a few hundred KB and arrive fast.

  Run from an ELEVATED PowerShell. Start it BEFORE launching the vendor
  updater, and stop it only after the updater reports success and the mouse has
  reconnected and been idle for a few seconds.

.EXAMPLE
  .\capture-firmware.ps1 -Label op1w4k_v108_to_v110
#>
param(
  [Parameter(Mandatory = $true)][string]$Label,
  [string]$OutDir = 'C:\Users\jmeyer\Downloads\endgame\git\re\captures\firmware',
  [string]$Iface  = '\\.\USBPcap1'
)

$ErrorActionPreference = 'Stop'
$cmd = 'C:\Program Files\USBPcap\USBPcapCMD.exe'
if (-not (Test-Path $cmd)) { throw "USBPcapCMD not found at $cmd" }

$isAdmin = ([Security.Principal.WindowsPrincipal] `
            [Security.Principal.WindowsIdentity]::GetCurrent()
           ).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) { throw 'Run this from an elevated (Administrator) PowerShell.' }

New-Item -ItemType Directory -Force $OutDir | Out-Null
$safe = ($Label -replace '[^A-Za-z0-9_.-]', '_')
$file = Join-Path $OutDir ("fw_{0}.pcap" -f $safe)
if (Test-Path $file) { throw "$file already exists - pick another label rather than overwriting a one-shot capture." }

# No --devices filter: we must see the bootloader device that appears mid-session.
# 64 MB kernel buffer so a burst of firmware chunks cannot overrun it.
$args = @(
  '-d', $Iface,
  '-o', $file,
  '--snaplen', '65535',
  '--bufferlen', '67108864'
)

Write-Host ''
Write-Host '  FIRMWARE UPDATE CAPTURE' -ForegroundColor Yellow
Write-Host "  -> $file" -ForegroundColor Cyan
Write-Host ''
Write-Host '  Capturing ALL devices on this root hub, including ones that appear later.'
Write-Host ''

$proc = Start-Process -FilePath $cmd -ArgumentList $args -PassThru -WindowStyle Hidden
Start-Sleep -Milliseconds 1200
if ($proc.HasExited) { throw "USBPcapCMD exited immediately (code $($proc.ExitCode))." }

Write-Host '  RECORDING.' -ForegroundColor Green
Write-Host ''
Write-Host '  1. Leave this running.'
Write-Host '  2. Launch the vendor firmware updater and let it finish completely.'
Write-Host '  3. Wait for the mouse to reconnect and sit idle ~10 seconds.'
Write-Host '  4. Only then press ENTER here.'
Write-Host ''
Write-Host '  Do NOT unplug anything while this runs.' -ForegroundColor Yellow
Write-Host ''
Read-Host  '  Press ENTER when the update is completely finished'

Stop-Process -Id $proc.Id -Force
Start-Sleep -Milliseconds 600

$len = (Get-Item $file).Length
Write-Host ''
Write-Host ("  saved {0} ({1:n0} bytes)" -f (Split-Path $file -Leaf), $len) -ForegroundColor Cyan
if ($len -lt 100000) {
  Write-Warning ("Only {0:n0} bytes captured. A firmware image is a few hundred KB, so this looks short - do not overwrite it, but treat it as suspect." -f $len)
}
Write-Host ''
Write-Host '  Back it up now, before doing anything else with the device.' -ForegroundColor Yellow
Write-Host ''
