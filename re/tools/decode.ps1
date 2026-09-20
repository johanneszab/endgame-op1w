<#
.SYNOPSIS
  Decode HID feature-report traffic out of a USBPcap capture into a diffable text form.

.DESCRIPTION
  Extracts SET_REPORT / GET_REPORT control transfers and prints one line per packet:

      #<frame>  SET_FEATURE rid=A1  cmd=16 sub=0F len=1C idx=01 | <64 bytes hex>

  Output is deliberately stable and line-oriented so two captures can be diffed:

      .\decode.ps1 ..\captures\00_baseline.pcap  > a.txt
      .\decode.ps1 ..\captures\01_cpi1_1000.pcap > b.txt
      Compare-Object (gc a.txt) (gc b.txt)

.EXAMPLE
  .\decode.ps1 ..\captures\01_cpi1_1000.pcap
#>
param(
  [Parameter(Mandatory = $true)][string]$Pcap,
  [switch]$All,                 # show every control transfer, not just feature reports
  [string]$Tshark = 'C:\Program Files\Wireshark\tshark.exe'
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path $Tshark)) { throw "tshark not found at $Tshark" }
if (-not (Test-Path $Pcap))   { throw "capture not found: $Pcap" }

$fields = @(
  'frame.number', 'usb.setup.bmRequestType', 'usb.setup.bRequest',
  'usb.setup.wValue', 'usb.setup.wIndex', 'usb.setup.wLength',
  'usb.capdata', 'usbhid.data', 'usb.data_fragment', 'usb.control.Data'
)
$argv = @('-r', $Pcap, '-Y', 'usb.transfer_type == 0x02', '-T', 'fields', '-E', 'separator=|')
foreach ($f in $fields) { $argv += @('-e', $f) }

$rows = & $Tshark @argv 2>$null
if (-not $rows) { Write-Warning 'No control transfers in this capture.'; return }

$REQ = @{ 1 = 'GET'; 9 = 'SET' }
$TYP = @{ 1 = 'INPUT'; 2 = 'OUTPUT'; 3 = 'FEATURE' }

foreach ($row in $rows) {
  $c = $row -split '\|'
  $frame = $c[0]
  if (-not $c[2]) { continue }                       # no setup stage -> completion packet

  $bReq   = [Convert]::ToInt32($c[2], 16)
  $wValue = if ($c[3]) { [Convert]::ToInt32(($c[3] -replace '^0x', ''), 16) } else { 0 }
  $rid    = $wValue -band 0xFF
  $rtype  = ($wValue -shr 8) -band 0xFF

  if (-not $All -and $rtype -ne 3) { continue }      # feature reports only

  # first non-empty data field wins
  $hex = ''
  foreach ($i in 6, 7, 8, 9) { if ($c[$i]) { $hex = $c[$i]; break } }
  $bytes = @()
  if ($hex) { $bytes = ($hex -replace '[^0-9a-fA-F]', '') -split '(..)' | Where-Object { $_ } }

  $verb = if ($REQ.ContainsKey($bReq)) { $REQ[$bReq] } else { "req$bReq" }
  $kind = if ($TYP.ContainsKey($rtype)) { $TYP[$rtype] } else { "type$rtype" }

  $ann = ''
  if ($bytes.Count -ge 7) {
    if ($verb -eq 'SET') {
      $ann = ('cmd={0} sub={1} len={2} idx={3}' -f $bytes[1], $bytes[2], $bytes[3], $bytes[6])
    } else {
      $ann = ('status={0}' -f $bytes[1])
    }
  }

  $dump = if ($bytes.Count) { ($bytes -join ' ') } else { '(no data captured)' }
  '{0,5}  {1}_{2} rid={3:X2}  {4,-34} | {5}' -f "#$frame", $verb, $kind, $rid, $ann, $dump
}
