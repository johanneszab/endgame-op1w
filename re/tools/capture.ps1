<#
.SYNOPSIS
  Capture one USB session per settings change, for differential protocol analysis.

.DESCRIPTION
  Run from an ELEVATED PowerShell (USBPcap needs Administrator).
  Each invocation records exactly one action, so the resulting pcap files can be
  diffed against each other to locate the byte that encodes a given setting.

.EXAMPLE
  .\capture.ps1 -Label baseline
  .\capture.ps1 -Label cpi1_400to1000
#>
param(
  [Parameter(Mandatory = $true)][string]$Label,
  [string]$OutDir  = 'C:\Users\jmeyer\Downloads\endgame\re\captures',
  [string]$Iface   = '\\.\USBPcap1',
  [int]   $Device  = 2      # the OP1w dongle (USB Composite Device) on USBPcap1
)

$ErrorActionPreference = 'Stop'
$cmd = 'C:\Program Files\USBPcap\USBPcapCMD.exe'
if (-not (Test-Path $cmd)) { throw "USBPcapCMD not found at $cmd" }

$isAdmin = ([Security.Principal.WindowsPrincipal] `
            [Security.Principal.WindowsIdentity]::GetCurrent()
           ).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) { throw 'Run this from an elevated (Administrator) PowerShell.' }

New-Item -ItemType Directory -Force $OutDir | Out-Null

$n    = @(Get-ChildItem $OutDir -Filter '*.pcap' -ErrorAction SilentlyContinue).Count
$safe = ($Label -replace '[^A-Za-z0-9_.-]', '_')
$file = Join-Path $OutDir ('{0:d2}_{1}.pcap' -f $n, $safe)

Write-Host ''
Write-Host "  capture -> $file" -ForegroundColor Cyan
Write-Host ''

$args = @('-d', $Iface, '-o', $file, '--devices', "$Device")
$proc = Start-Process -FilePath $cmd -ArgumentList $args -PassThru -WindowStyle Hidden
Start-Sleep -Milliseconds 700           # let the driver attach before the action

if ($proc.HasExited) { throw "USBPcapCMD exited immediately (code $($proc.ExitCode)). Is the device number right?" }

Write-Host '  RECORDING.' -ForegroundColor Green
Write-Host '  Perform exactly ONE action in the configuration tool now (then click APPLY).'
Read-Host  '  Press ENTER when the action is complete'

Stop-Process -Id $proc.Id -Force
Start-Sleep -Milliseconds 400

$len = (Get-Item $file).Length
Write-Host ''
Write-Host ("  saved {0} ({1:n0} bytes)" -f (Split-Path $file -Leaf), $len) -ForegroundColor Cyan
if ($len -lt 200) { Write-Warning 'File is suspiciously small - no traffic captured?' }
Write-Host ''
